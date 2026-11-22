#pragma once

#include "math_defs.h"

namespace nle {

struct Vector4 final {
    real_t x = 0;
    real_t y = 0;
    real_t z = 0;
    real_t w = 0;

    constexpr Vector4() noexcept = default;
    constexpr Vector4(
        const real_t x_value,
        const real_t y_value,
        const real_t z_value,
        const real_t w_value
    ) noexcept : x(x_value), y(y_value), z(z_value), w(w_value) {}

    [[nodiscard]] constexpr real_t dot(const Vector4 other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    [[nodiscard]] constexpr real_t length_squared() const noexcept {
        return dot(*this);
    }

    constexpr Vector4& operator+=(const Vector4 other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    constexpr Vector4& operator-=(const Vector4 other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    constexpr Vector4& operator*=(const real_t scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    friend constexpr Vector4 operator+(Vector4 left, const Vector4 right) noexcept {
        return left += right;
    }

    friend constexpr Vector4 operator-(Vector4 left, const Vector4 right) noexcept {
        return left -= right;
    }

    friend constexpr Vector4 operator*(Vector4 vector, const real_t scalar) noexcept {
        return vector *= scalar;
    }

    friend constexpr Vector4 operator*(const real_t scalar, Vector4 vector) noexcept {
        return vector *= scalar;
    }

    friend constexpr bool operator==(const Vector4&, const Vector4&) noexcept = default;
};

} // namespace nle
