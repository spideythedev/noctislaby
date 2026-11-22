#pragma once

#include "resource.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nle {

class ResourceCache final {
public:
    [[nodiscard]] bool insert(const Ref<Resource>& resource);
    [[nodiscard]] Ref<Resource> get(std::string_view path) const;
    [[nodiscard]] bool contains(std::string_view path) const;
    [[nodiscard]] bool remove(std::string_view path);
    [[nodiscard]] std::size_t size() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Ref<Resource>> resources_;
};

} // namespace nle
