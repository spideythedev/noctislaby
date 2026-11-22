#pragma once

#include <cstdint>

namespace nle {

struct ResourceUID final {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value != 0;
    }

    explicit constexpr operator bool() const noexcept {
        return is_valid();
    }

    friend constexpr bool operator==(const ResourceUID&, const ResourceUID&) noexcept = default;
};

} // namespace nle
