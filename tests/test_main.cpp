#include "error/error.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    using nle::ErrorCode;

    expect(nle::error_code_name(ErrorCode::Ok) == "Ok", "Ok error code name");
    expect(nle::error_code_name(ErrorCode::InvalidParameter) == "InvalidParameter",
           "InvalidParameter error code name");
    expect(nle::error_code_name(ErrorCode::PlatformError) == "PlatformError",
           "PlatformError error code name");

    std::cout << "All NoctisLaby core tests passed.\n";
    return EXIT_SUCCESS;
}
