#pragma once

#include <cstdint>
#include <functional>

namespace nle {

class RID final {
public:
    constexpr RID() noexcept = default;

    [[nodiscard]] static constexpr RID from_parts(
        const std::uint32_t index,
        const std::uint32_t generation
    ) noexcept {
        return RID{
            (static_cast<std::uint64_t>(generation) << 32U) |
            (static_cast<std::uint64_t>(index) + 1U)
        };
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value_ != 0;
    }

    [[nodiscard]] constexpr std::uint32_t get_index() const noexcept {
        return static_cast<std::uint32_t>(value_) - 1U;
    }

    [[nodiscard]] constexpr std::uint32_t get_generation() const noexcept {
        return static_cast<std::uint32_t>(value_ >> 32U);
    }

    [[nodiscard]] constexpr std::uint64_t value() const noexcept {
        return value_;
    }

    friend constexpr bool operator==(const RID&, const RID&) noexcept = default;

private:
    explicit constexpr RID(const std::uint64_t value) noexcept : value_(value) {}

    std::uint64_t value_ = 0;
};

} // namespace nle

template <>
struct std::hash<nle::RID> {
    [[nodiscard]] std::size_t operator()(const nle::RID rid) const noexcept {
        return std::hash<std::uint64_t>{}(rid.value());
    }
};
