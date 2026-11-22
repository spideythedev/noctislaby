#include "resource_cache.h"
#include "resource_path.h"

#include <utility>

namespace nle {

bool ResourceCache::insert(const Ref<Resource>& resource) {
    if (!resource) {
        return false;
    }

    std::string path;
    if (ResourcePath::normalize(resource->get_path(), path) != ErrorCode::Ok) {
        return false;
    }

    std::lock_guard lock(mutex_);
    return resources_.emplace(path, resource).second;
}

Ref<Resource> ResourceCache::get(const std::string_view path) const {
    std::string normalized;
    if (ResourcePath::normalize(path, normalized) != ErrorCode::Ok) {
        return {};
    }

    std::lock_guard lock(mutex_);

    const auto found = resources_.find(normalized);
    if (found == resources_.end()) {
        return {};
    }

    return Ref<Resource>(found->second.get());
}

bool ResourceCache::contains(const std::string_view path) const {
    std::string normalized;
    if (ResourcePath::normalize(path, normalized) != ErrorCode::Ok) {
        return false;
    }

    std::lock_guard lock(mutex_);
    return resources_.contains(normalized);
}

bool ResourceCache::remove(const std::string_view path) {
    std::string normalized;
    if (ResourcePath::normalize(path, normalized) != ErrorCode::Ok) {
        return false;
    }

    Ref<Resource> removed;

    {
        std::lock_guard lock(mutex_);

        const auto found = resources_.find(normalized);
        if (found == resources_.end()) {
            return false;
        }

        removed = std::move(found->second);
        resources_.erase(found);
    }

    return true;
}

std::size_t ResourceCache::size() const {
    std::lock_guard lock(mutex_);
    return resources_.size();
}

void ResourceCache::clear() {
    std::unordered_map<std::string, Ref<Resource>> removed;

    {
        std::lock_guard lock(mutex_);
        removed.swap(resources_);
    }
}

} // namespace nle
