#include "resource_cache.h"

#include <utility>

namespace nle {

bool ResourceCache::insert(const Ref<Resource>& resource) {
    if (!resource) {
        return false;
    }

    const std::string path(resource->get_path());
    if (path.empty()) {
        return false;
    }

    std::lock_guard lock(mutex_);
    return resources_.emplace(path, resource).second;
}

Ref<Resource> ResourceCache::get(const std::string_view path) const {
    std::lock_guard lock(mutex_);

    const auto found = resources_.find(std::string(path));
    if (found == resources_.end()) {
        return {};
    }

    return Ref<Resource>(found->second.get());
}

bool ResourceCache::contains(const std::string_view path) const {
    std::lock_guard lock(mutex_);
    return resources_.contains(std::string(path));
}

bool ResourceCache::remove(const std::string_view path) {
    Ref<Resource> removed;

    {
        std::lock_guard lock(mutex_);

        const auto found = resources_.find(std::string(path));
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
