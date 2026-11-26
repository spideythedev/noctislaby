#pragma once

#include "error/error.h"
#include "resource.h"

#include <string>
#include <string_view>
#include <vector>

namespace nle {

class ResourceFormatLoader {
public:
    virtual ~ResourceFormatLoader() = default;

    [[nodiscard]] virtual std::vector<std::string> get_recognized_extensions() const = 0;
    [[nodiscard]] virtual ErrorCode get_dependencies(
        std::string_view path,
        std::vector<std::string>& dependencies
    ) const {
        (void)path;
        dependencies.clear();
        return ErrorCode::Ok;
    }

    [[nodiscard]] virtual Ref<Resource> load(
        std::string_view path,
        ErrorCode& error
    ) = 0;
};

} // namespace nle
