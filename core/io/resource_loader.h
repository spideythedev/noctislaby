#pragma once

#include "resource_cache.h"
#include "resource_format_loader.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nle {

class ResourceLoader final {
public:
    [[nodiscard]] bool add_format_loader(std::shared_ptr<ResourceFormatLoader> loader);
    [[nodiscard]] Ref<Resource> load(
        std::string_view path,
        ErrorCode* error = nullptr
    );

    [[nodiscard]] ResourceCache& get_cache() noexcept;
    [[nodiscard]] const ResourceCache& get_cache() const noexcept;

private:
    struct LoadState final {
        std::condition_variable condition;
        bool completed = false;
        Ref<Resource> resource;
        ErrorCode error = ErrorCode::Failed;
    };

    [[nodiscard]] std::shared_ptr<ResourceFormatLoader> find_loader(
        std::string_view extension
    ) const;

    mutable std::mutex loaders_mutex_;
    std::vector<std::shared_ptr<ResourceFormatLoader>> loaders_;
    ResourceCache cache_;
    std::mutex loads_mutex_;
    std::unordered_map<std::string, std::shared_ptr<LoadState>> in_flight_;
};

} // namespace nle
