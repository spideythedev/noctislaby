#include "resource_loader.h"
#include "resource_path.h"

#include <algorithm>
#include <cctype>
#include <new>
#include <string>
#include <utility>

namespace nle {
namespace {

std::string get_extension(const std::string_view path) {
    const std::size_t separator = path.find_last_of("/\\");
    const std::size_t dot = path.find_last_of('.');

    if (dot == std::string_view::npos ||
        (separator != std::string_view::npos && dot < separator)) {
        return {};
    }

    std::string extension(path.substr(dot + 1));
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );

    return extension;
}

} // namespace

bool ResourceLoader::add_format_loader(std::shared_ptr<ResourceFormatLoader> loader) {
    if (loader == nullptr) {
        return false;
    }

    std::lock_guard lock(loaders_mutex_);

    const auto found = std::find(loaders_.begin(), loaders_.end(), loader);
    if (found != loaders_.end()) {
        return false;
    }

    loaders_.push_back(std::move(loader));
    return true;
}

Ref<Resource> ResourceLoader::load(
    const std::string_view path,
    ErrorCode* error
) {
    std::string normalized_path;
    if (ResourcePath::normalize(path, normalized_path) != ErrorCode::Ok) {
        if (error != nullptr) {
            *error = ErrorCode::InvalidParameter;
        }
        return {};
    }

    if (Ref<Resource> cached = cache_.get(normalized_path)) {
        if (error != nullptr) {
            *error = ErrorCode::Ok;
        }
        return cached;
    }

    std::shared_ptr<LoadState> state;
    {
        std::unique_lock lock(loads_mutex_);

        if (Ref<Resource> cached = cache_.get(normalized_path)) {
            if (error != nullptr) {
                *error = ErrorCode::Ok;
            }
            return cached;
        }

        const auto found = in_flight_.find(normalized_path);
        if (found != in_flight_.end()) {
            state = found->second;
            state->condition.wait(lock, [&state] {
                return state->completed;
            });

            if (error != nullptr) {
                *error = state->error;
            }
            return state->resource;
        }

        try {
            state = std::make_shared<LoadState>();
            in_flight_.emplace(normalized_path, state);
        } catch (const std::bad_alloc&) {
            if (error != nullptr) {
                *error = ErrorCode::OutOfMemory;
            }
            return {};
        }
    }

    Ref<Resource> resource;
    ErrorCode load_error = ErrorCode::Ok;

    try {
        const std::string extension = get_extension(normalized_path);
        const std::shared_ptr<ResourceFormatLoader> loader = find_loader(extension);

        if (loader == nullptr) {
            load_error = ErrorCode::NotFound;
        } else {
            resource = loader->load(normalized_path, load_error);
            if (!resource && load_error == ErrorCode::Ok) {
                load_error = ErrorCode::Failed;
            }
        }

        if (resource) {
            if (resource->get_path() != normalized_path) {
                resource->set_path(normalized_path);
            }

            if (!cache_.insert(resource)) {
                Ref<Resource> existing = cache_.get(normalized_path);
                if (existing) {
                    resource = std::move(existing);
                } else {
                    resource.reset();
                    load_error = ErrorCode::Failed;
                }
            }
        }
    } catch (const std::bad_alloc&) {
        resource.reset();
        load_error = ErrorCode::OutOfMemory;
    } catch (...) {
        resource.reset();
        load_error = ErrorCode::Failed;
    }

    {
        std::lock_guard lock(loads_mutex_);
        state->resource = resource;
        state->error = resource ? ErrorCode::Ok : load_error;
        state->completed = true;
        in_flight_.erase(normalized_path);
    }
    state->condition.notify_all();

    if (error != nullptr) {
        *error = state->error;
    }
    return resource;
}

ResourceCache& ResourceLoader::get_cache() noexcept {
    return cache_;
}

const ResourceCache& ResourceLoader::get_cache() const noexcept {
    return cache_;
}

std::shared_ptr<ResourceFormatLoader> ResourceLoader::find_loader(
    const std::string_view extension
) const {
    std::vector<std::shared_ptr<ResourceFormatLoader>> loaders;
    {
        std::lock_guard lock(loaders_mutex_);
        loaders = loaders_;
    }

    for (const auto& loader : loaders) {
        const std::vector<std::string> extensions = loader->get_recognized_extensions();

        const auto found = std::find_if(
            extensions.begin(),
            extensions.end(),
            [extension](const std::string& candidate) {
                if (candidate.size() != extension.size()) {
                    return false;
                }

                return std::equal(
                    candidate.begin(),
                    candidate.end(),
                    extension.begin(),
                    [](const char left, const char right) {
                        return std::tolower(static_cast<unsigned char>(left)) ==
                               std::tolower(static_cast<unsigned char>(right));
                    }
                );
            }
        );

        if (found != extensions.end()) {
            return loader;
        }
    }

    return {};
}

} // namespace nle
