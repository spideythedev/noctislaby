#pragma once

#include "vector3.h"

#include <algorithm>
#include <cmath>

namespace nle {

struct Quaternion final {
    real_t x = 0;
    real_t y = 0;
    real_t z = 0;
    real_t w = 1;

    constexpr Quaternion() noexcept = default;
    constexpr Quaternion(
        const real_t x_value,
        const real_t y_value,
        const real_t z_value,
        const real_t w_value
    ) noexcept : x(x_value), y(y_value), z(z_value), w(w_value) {}

    [[nodiscard]] static Quaternion from_axis_angle(
        const Vector3 axis,
        const real_t angle
    ) noexcept {
        const Vector3 normalized_axis = axis.normalized();
        const real_t half_angle = angle * real_t{0.5};
        const real_t sine = std::sin(half_angle);

        return {
            normalized_axis.x * sine,
            normalized_axis.y * sine,
            normalized_axis.z * sine,
            std::cos(half_angle)
        };
    }

    [[nodiscard]] constexpr real_t dot(const Quaternion other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    [[nodiscard]] constexpr real_t length_squared() const noexcept {
        return dot(*this);
    }

    [[nodiscard]] real_t length() const noexcept {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] Quaternion normalized() const noexcept {
        const real_t magnitude = length();
        if (magnitude <= REAL_EPSILON) {
            return {};
        }

        return {x / magnitude, y / magnitude, z / magnitude, w / magnitude};
    }

    [[nodiscard]] constexpr Quaternion conjugated() const noexcept {
        return {-x, -y, -z, w};
    }

    [[nodiscard]] Quaternion inverse() const noexcept {
        const real_t magnitude_squared = length_squared();
        if (magnitude_squared <= REAL_EPSILON) {
            return {};
        }

        const Quaternion conjugate = conjugated();
        return {
            conjugate.x / magnitude_squared,
            conjugate.y / magnitude_squared,
            conjugate.z / magnitude_squared,
            conjugate.w / magnitude_squared
        };
    }

    [[nodiscard]] Vector3 xform(const Vector3 vector) const noexcept {
        const Vector3 imaginary{x, y, z};
        const Vector3 twice_cross = imaginary.cross(vector) * real_t{2};
        return vector + twice_cross * w + imaginary.cross(twice_cross);
    }

    [[nodiscard]] static Quaternion slerp(
        Quaternion from,
        Quaternion to,
        const real_t weight
    ) noexcept {
        from = from.normalized();
        to = to.normalized();

        real_t cosine = from.dot(to);
        if (cosine < 0) {
            to = {-to.x, -to.y, -to.z, -to.w};
            cosine = -cosine;
        }

        cosine = std::clamp(cosine, real_t{-1}, real_t{1});

        if (cosine > real_t{0.9995}) {
            return Quaternion{
                from.x + (to.x - from.x) * weight,
                from.y + (to.y - from.y) * weight,
                from.z + (to.z - from.z) * weight,
                from.w + (to.w - from.w) * weight
            }.normalized();
        }

        const real_t angle = std::acos(cosine);
        const real_t sine = std::sin(angle);
        const real_t from_weight = std::sin((real_t{1} - weight) * angle) / sine;
        const real_t to_weight = std::sin(weight * angle) / sine;

        return {
            from.x * from_weight + to.x * to_weight,
            from.y * from_weight + to.y * to_weight,
            from.z * from_weight + to.z * to_weight,
            from.w * from_weight + to.w * to_weight
        };
    }

    friend constexpr Quaternion operator*(const Quaternion left, const Quaternion right) noexcept {
        return {
            left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
            left.w * right.y - left.x * right.z + left.y * right.w + left.z * right.x,
            left.w * right.z + left.x * right.y - left.y * right.x + left.z * right.w,
            left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z
        };
    }

    friend constexpr bool operator==(const Quaternion&, const Quaternion&) noexcept = default;
};

} // namespace nle
