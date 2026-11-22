#pragma once

#include "error/error.h"
#include "resource.h"

#include <string>
#include <string_view>
#include <vector>

namespace nle {

class ResourceFormatSaver {
public:
    virtual ~ResourceFormatSaver() = default;

    [[nodiscard]] virtual std::vector<std::string> get_recognized_extensions() const = 0;
    [[nodiscard]] virtual ErrorCode save(
        std::string_view path,
        const Ref<Resource>& resource
    ) = 0;
};

} // namespace nle
