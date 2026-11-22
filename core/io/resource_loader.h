#pragma once

#include "resource_cache.h"
#include "resource_format_loader.h"

#include <memory>
#include <mutex>
#include <string_view>
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
    [[nodiscard]] std::shared_ptr<ResourceFormatLoader> find_loader(
        std::string_view extension
    ) const;

    mutable std::mutex loaders_mutex_;
    std::vector<std::shared_ptr<ResourceFormatLoader>> loaders_;
    ResourceCache cache_;
};

} // namespace nle
