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

ResourceLoadRequest::ResourceLoadRequest(std::shared_ptr<State> state) : state_(std::move(state)) {}
bool ResourceLoadRequest::is_valid() const noexcept { return static_cast<bool>(state_); }
ResourceLoadRequest::Status ResourceLoadRequest::get_status() const {
    if (!state_) return Status::Invalid;
    std::lock_guard lock(state_->mutex);
    return state_->status;
}
ErrorCode ResourceLoadRequest::get_error() const {
    if (!state_) return ErrorCode::InvalidState;
    std::lock_guard lock(state_->mutex);
    return state_->error;
}
Ref<Resource> ResourceLoadRequest::get_resource() const {
    if (!state_) return {};
    std::lock_guard lock(state_->mutex);
    return state_->status == Status::Loaded ? state_->resource : Ref<Resource>{};
}
Ref<Resource> ResourceLoadRequest::wait(ErrorCode* error) const {
    if (!state_) {
        if (error) *error = ErrorCode::InvalidState;
        return {};
    }
    std::unique_lock lock(state_->mutex);
    state_->condition.wait(lock, [this] {
        return state_->status == Status::Loaded || state_->status == Status::Failed;
    });
    if (error) *error = state_->error;
    return state_->resource;
}
ResourceLoader::~ResourceLoader() {
    {
        std::lock_guard lock(worker_mutex_);
        stopping_workers_ = true;
    }
    worker_condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

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

ResourceLoadRequest ResourceLoader::load_async(const std::string_view path) {
    std::shared_ptr<ResourceLoadRequest::State> state;
    try {
        state = std::make_shared<ResourceLoadRequest::State>();
    } catch (const std::bad_alloc&) {
        return {};
    }
    ResourceLoadRequest request(state);
    std::string normalized_path;
    if (ResourcePath::normalize(path, normalized_path) != ErrorCode::Ok) {
        std::lock_guard lock(state->mutex);
        state->status = ResourceLoadRequest::Status::Failed;
        state->error = ErrorCode::InvalidParameter;
        return request;
    }
    if (Ref<Resource> cached = cache_.get(normalized_path)) {
        std::lock_guard lock(state->mutex);
        state->resource = std::move(cached);
        state->status = ResourceLoadRequest::Status::Loaded;
        state->error = ErrorCode::Ok;
        return request;
    }
    {
        std::lock_guard lock(worker_mutex_);
        if (stopping_workers_) {
            std::lock_guard state_lock(state->mutex);
            state->status = ResourceLoadRequest::Status::Failed;
            state->error = ErrorCode::InvalidState;
            return request;
        }
        try {
            if (workers_.empty()) {
                workers_.reserve(2);
                workers_.emplace_back(&ResourceLoader::worker_loop, this);
                try {
                    workers_.emplace_back(&ResourceLoader::worker_loop, this);
                } catch (const std::system_error&) {
                    // One worker is sufficient when the platform cannot start a second.
                }
            }
            async_tasks_.push_back({std::move(normalized_path), state});
        } catch (const std::bad_alloc&) {
            std::lock_guard state_lock(state->mutex);
            state->status = ResourceLoadRequest::Status::Failed;
            state->error = ErrorCode::OutOfMemory;
            return request;
        } catch (const std::system_error&) {
            std::lock_guard state_lock(state->mutex);
            state->status = ResourceLoadRequest::Status::Failed;
            state->error = ErrorCode::PlatformError;
            return request;
        }
    }
    worker_condition_.notify_one();
    return request;
}
void ResourceLoader::worker_loop() {
    while (true) {
        AsyncTask task;
        {
            std::unique_lock lock(worker_mutex_);
            worker_condition_.wait(lock, [this] {
                return stopping_workers_ || !async_tasks_.empty();
            });
            if (stopping_workers_ && async_tasks_.empty()) return;
            task = std::move(async_tasks_.front());
            async_tasks_.pop_front();
        }
        {
            std::lock_guard lock(task.state->mutex);
            task.state->status = ResourceLoadRequest::Status::Loading;
        }
        ErrorCode error = ErrorCode::Failed;
        Ref<Resource> resource = load(task.path, &error);
        {
            std::lock_guard lock(task.state->mutex);
            task.state->resource = std::move(resource);
            task.state->error = task.state->resource ? ErrorCode::Ok : error;
            task.state->status = task.state->resource
                ? ResourceLoadRequest::Status::Loaded
                : ResourceLoadRequest::Status::Failed;
        }
        task.state->condition.notify_all();
    }
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
