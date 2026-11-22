#pragma once

#include "error/error.h"

#include <string>
#include <string_view>

namespace nle {

class ResourcePath final {
public:
    [[nodiscard]] static ErrorCode normalize(
        std::string_view path,
        std::string& normalized
    );
};

} // namespace nle
