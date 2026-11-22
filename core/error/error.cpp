#include "error.h"

namespace nle {

std::string_view error_code_name(const ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Ok: return "Ok";
        case ErrorCode::Failed: return "Failed";
        case ErrorCode::InvalidParameter: return "InvalidParameter";
        case ErrorCode::InvalidState: return "InvalidState";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::AlreadyExists: return "AlreadyExists";
        case ErrorCode::NotImplemented: return "NotImplemented";
        case ErrorCode::OutOfMemory: return "OutOfMemory";
        case ErrorCode::IoError: return "IoError";
        case ErrorCode::PlatformError: return "PlatformError";
    }

    return "Unknown";
}

} // namespace nle
