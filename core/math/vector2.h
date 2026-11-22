#pragma once

#include "math_defs.h"

#include <cmath>

namespace nle {

struct Vector2 final {
    real_t x = 0;
    real_t y = 0;

    constexpr Vector2() noexcept = default;
    constexpr Vector2(const real_t x_value, const real_t y_value) noexcept
        : x(x_value), y(y_value) {}

    [[nodiscard]] constexpr real_t dot(const Vector2 other) const noexcept {
        return x * other.x + y * other.y;
    }

    [[nodiscard]] constexpr real_t cross(const Vector2 other) const noexcept {
        return x * other.y - y * other.x;
    }

    [[nodiscard]] constexpr real_t length_squared() const noexcept {
        return dot(*this);
    }

    [[nodiscard]] real_t length() const noexcept {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] Vector2 normalized() const noexcept {
        const real_t magnitude = length();
        return magnitude > REAL_EPSILON ? *this / magnitude : Vector2{};
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return x == 0 && y == 0;
    }

    constexpr Vector2& operator+=(const Vector2 other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2& operator-=(const Vector2 other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector2& operator*=(const real_t scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2& operator/=(const real_t scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    friend constexpr Vector2 operator+(Vector2 left, const Vector2 right) noexcept {
        return left += right;
    }

    friend constexpr Vector2 operator-(Vector2 left, const Vector2 right) noexcept {
        return left -= right;
    }

    friend constexpr Vector2 operator*(Vector2 vector, const real_t scalar) noexcept {
        return vector *= scalar;
    }

    friend constexpr Vector2 operator*(const real_t scalar, Vector2 vector) noexcept {
        return vector *= scalar;
    }

    friend constexpr Vector2 operator/(Vector2 vector, const real_t scalar) noexcept {
        return vector /= scalar;
    }

    friend constexpr Vector2 operator-(const Vector2 vector) noexcept {
        return {-vector.x, -vector.y};
    }

    friend constexpr bool operator==(const Vector2&, const Vector2&) noexcept = default;
};

} // namespace nle
