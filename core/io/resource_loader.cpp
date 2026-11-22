#include "resource_loader.h"

#include <algorithm>
#include <cctype>
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
    if (path.empty()) {
        if (error != nullptr) {
            *error = ErrorCode::InvalidParameter;
        }
        return {};
    }

    if (Ref<Resource> cached = cache_.get(path)) {
        if (error != nullptr) {
            *error = ErrorCode::Ok;
        }
        return cached;
    }

    const std::string extension = get_extension(path);
    const std::shared_ptr<ResourceFormatLoader> loader = find_loader(extension);

    if (loader == nullptr) {
        if (error != nullptr) {
            *error = ErrorCode::NotFound;
        }
        return {};
    }

    ErrorCode load_error = ErrorCode::Ok;
    Ref<Resource> resource = loader->load(path, load_error);

    if (!resource) {
        if (error != nullptr) {
            *error = load_error == ErrorCode::Ok ? ErrorCode::Failed : load_error;
        }
        return {};
    }

    if (resource->get_path().empty()) {
        resource->set_path(std::string(path));
    }

    if (!cache_.insert(resource)) {
        Ref<Resource> existing = cache_.get(path);
        if (existing) {
            if (error != nullptr) {
                *error = ErrorCode::Ok;
            }
            return existing;
        }
    }

    if (error != nullptr) {
        *error = ErrorCode::Ok;
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
    std::lock_guard lock(loaders_mutex_);

    for (const auto& loader : loaders_) {
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
