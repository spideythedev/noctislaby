#pragma once

#include "error/error.h"
#include "variant.h"

#include <cstddef>
#include <span>
#include <vector>

namespace nle {

class VariantSerializer final {
public:
    [[nodiscard]] static ErrorCode serialize(
        const Variant& value,
        std::vector<std::byte>& output
    );
    [[nodiscard]] static ErrorCode deserialize(
        std::span<const std::byte> input,
        Variant& value
    );
};

} // namespace nle
