#pragma once

#include <string_view>

namespace nle {

enum class ErrorCode {
    Ok = 0,
    Failed,
    InvalidParameter,
    InvalidState,
    NotFound,
    AlreadyExists,
    NotImplemented,
    OutOfMemory,
    IoError,
    PlatformError
};

[[nodiscard]] std::string_view error_code_name(ErrorCode code) noexcept;

} // namespace nle
