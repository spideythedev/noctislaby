#pragma once

#include "math_defs.h"

#include <algorithm>
#include <cmath>

namespace nle {

struct Color final {
    real_t r = 0;
    real_t g = 0;
    real_t b = 0;
    real_t a = 1;

    constexpr Color() noexcept = default;
    constexpr Color(
        const real_t red,
        const real_t green,
        const real_t blue,
        const real_t alpha = 1
    ) noexcept : r(red), g(green), b(blue), a(alpha) {}

    [[nodiscard]] static Color from_srgb(const Color color) noexcept {
        const auto convert = [](const real_t channel) {
            return channel <= real_t{0.04045}
                ? channel / real_t{12.92}
                : std::pow((channel + real_t{0.055}) / real_t{1.055}, real_t{2.4});
        };

        return {convert(color.r), convert(color.g), convert(color.b), color.a};
    }

    [[nodiscard]] Color to_srgb() const noexcept {
        const auto convert = [](const real_t channel) {
            return channel <= real_t{0.0031308}
                ? channel * real_t{12.92}
                : real_t{1.055} * std::pow(channel, real_t{1} / real_t{2.4}) -
                      real_t{0.055};
        };

        return {convert(r), convert(g), convert(b), a};
    }

    [[nodiscard]] constexpr Color clamped(
        const real_t minimum = 0,
        const real_t maximum = 1
    ) const noexcept {
        return {
            std::clamp(r, minimum, maximum),
            std::clamp(g, minimum, maximum),
            std::clamp(b, minimum, maximum),
            std::clamp(a, minimum, maximum)
        };
    }

    [[nodiscard]] constexpr Color premultiplied() const noexcept {
        return {r * a, g * a, b * a, a};
    }

    [[nodiscard]] static constexpr Color lerp(
        const Color from,
        const Color to,
        const real_t weight
    ) noexcept {
        return {
            from.r + (to.r - from.r) * weight,
            from.g + (to.g - from.g) * weight,
            from.b + (to.b - from.b) * weight,
            from.a + (to.a - from.a) * weight
        };
    }

    friend constexpr Color operator+(const Color left, const Color right) noexcept {
        return {
            left.r + right.r,
            left.g + right.g,
            left.b + right.b,
            left.a + right.a
        };
    }

    friend constexpr Color operator*(const Color color, const real_t scalar) noexcept {
        return {
            color.r * scalar,
            color.g * scalar,
            color.b * scalar,
            color.a * scalar
        };
    }

    friend constexpr bool operator==(const Color&, const Color&) noexcept = default;
};

} // namespace nle
