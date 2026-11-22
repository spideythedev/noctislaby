#pragma once

#include "vector3.h"

#include <cmath>

namespace nle {

struct Plane final {
    Vector3 normal{0, 1, 0};
    real_t d = 0;

    constexpr Plane() noexcept = default;
    constexpr Plane(const Vector3 normal_value, const real_t distance) noexcept
        : normal(normal_value), d(distance) {}

    [[nodiscard]] static constexpr Plane from_point_normal(
        const Vector3 point,
        const Vector3 normal
    ) noexcept {
        return {normal, -normal.dot(point)};
    }

    [[nodiscard]] constexpr real_t distance_to(const Vector3 point) const noexcept {
        return normal.dot(point) + d;
    }

    [[nodiscard]] constexpr bool has_point(
        const Vector3 point,
        const real_t tolerance = REAL_EPSILON
    ) const noexcept {
        const real_t distance = distance_to(point);
        return distance >= -tolerance && distance <= tolerance;
    }

    [[nodiscard]] constexpr Vector3 project(const Vector3 point) const noexcept {
        const real_t normal_length_squared = normal.length_squared();
        if (normal_length_squared <= REAL_EPSILON) {
            return point;
        }

        return point - normal * (distance_to(point) / normal_length_squared);
    }

    [[nodiscard]] bool intersect_ray(
        const Vector3 origin,
        const Vector3 direction,
        Vector3& intersection
    ) const noexcept {
        const real_t denominator = normal.dot(direction);
        if (std::abs(denominator) <= REAL_EPSILON) {
            return false;
        }

        const real_t distance = -distance_to(origin) / denominator;
        if (distance < 0) {
            return false;
        }

        intersection = origin + direction * distance;
        return true;
    }

    friend constexpr bool operator==(const Plane&, const Plane&) noexcept = default;
};

} // namespace nle
