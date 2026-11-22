#pragma once

#include <cstdint>
#include <functional>

namespace nle {

struct ObjectID final {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value != 0;
    }

    explicit constexpr operator bool() const noexcept {
        return is_valid();
    }

    friend constexpr bool operator==(const ObjectID&, const ObjectID&) noexcept = default;
};

struct ObjectIDHash final {
    [[nodiscard]] std::size_t operator()(const ObjectID id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value);
    }
};

} // namespace nle
