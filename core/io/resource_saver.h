#pragma once

#include "resource_format_saver.h"

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace nle {

class ResourceSaver final {
public:
    [[nodiscard]] bool add_format_saver(std::shared_ptr<ResourceFormatSaver> saver);
    [[nodiscard]] ErrorCode save(
        std::string_view path,
        const Ref<Resource>& resource
    );

private:
    [[nodiscard]] std::shared_ptr<ResourceFormatSaver> find_saver(
        std::string_view extension
    ) const;

    mutable std::mutex savers_mutex_;
    std::vector<std::shared_ptr<ResourceFormatSaver>> savers_;
};

} // namespace nle
