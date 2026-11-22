#pragma once

#include "math_defs.h"

#include <cmath>

namespace nle {

struct Vector3 final {
    real_t x = 0;
    real_t y = 0;
    real_t z = 0;

    constexpr Vector3() noexcept = default;
    constexpr Vector3(
        const real_t x_value,
        const real_t y_value,
        const real_t z_value
    ) noexcept : x(x_value), y(y_value), z(z_value) {}

    [[nodiscard]] constexpr real_t dot(const Vector3 other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr Vector3 cross(const Vector3 other) const noexcept {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    [[nodiscard]] constexpr real_t length_squared() const noexcept {
        return dot(*this);
    }

    [[nodiscard]] real_t length() const noexcept {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] Vector3 normalized() const noexcept {
        const real_t magnitude = length();
        return magnitude > REAL_EPSILON ? *this / magnitude : Vector3{};
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return x == 0 && y == 0 && z == 0;
    }

    constexpr Vector3& operator+=(const Vector3 other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vector3& operator-=(const Vector3 other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vector3& operator*=(const real_t scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vector3& operator/=(const real_t scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    friend constexpr Vector3 operator+(Vector3 left, const Vector3 right) noexcept {
        return left += right;
    }

    friend constexpr Vector3 operator-(Vector3 left, const Vector3 right) noexcept {
        return left -= right;
    }

    friend constexpr Vector3 operator*(Vector3 vector, const real_t scalar) noexcept {
        return vector *= scalar;
    }

    friend constexpr Vector3 operator*(const real_t scalar, Vector3 vector) noexcept {
        return vector *= scalar;
    }

    friend constexpr Vector3 operator/(Vector3 vector, const real_t scalar) noexcept {
        return vector /= scalar;
    }

    friend constexpr Vector3 operator-(const Vector3 vector) noexcept {
        return {-vector.x, -vector.y, -vector.z};
    }

    friend constexpr bool operator==(const Vector3&, const Vector3&) noexcept = default;
};

} // namespace nle
