#pragma once

#include "quaternion.h"

#include <cmath>

namespace nle {

struct Basis final {
    Vector3 x{1, 0, 0};
    Vector3 y{0, 1, 0};
    Vector3 z{0, 0, 1};

    constexpr Basis() noexcept = default;
    constexpr Basis(const Vector3 x_axis, const Vector3 y_axis, const Vector3 z_axis) noexcept
        : x(x_axis), y(y_axis), z(z_axis) {}

    [[nodiscard]] static Basis from_quaternion(const Quaternion rotation) noexcept {
        const Quaternion q = rotation.normalized();

        const real_t xx = q.x * q.x;
        const real_t yy = q.y * q.y;
        const real_t zz = q.z * q.z;
        const real_t xy = q.x * q.y;
        const real_t xz = q.x * q.z;
        const real_t yz = q.y * q.z;
        const real_t wx = q.w * q.x;
        const real_t wy = q.w * q.y;
        const real_t wz = q.w * q.z;

        return {
            {
                real_t{1} - real_t{2} * (yy + zz),
                real_t{2} * (xy + wz),
                real_t{2} * (xz - wy)
            },
            {
                real_t{2} * (xy - wz),
                real_t{1} - real_t{2} * (xx + zz),
                real_t{2} * (yz + wx)
            },
            {
                real_t{2} * (xz + wy),
                real_t{2} * (yz - wx),
                real_t{1} - real_t{2} * (xx + yy)
            }
        };
    }

    [[nodiscard]] constexpr Vector3 xform(const Vector3 vector) const noexcept {
        return x * vector.x + y * vector.y + z * vector.z;
    }

    [[nodiscard]] constexpr real_t determinant() const noexcept {
        return x.dot(y.cross(z));
    }

    [[nodiscard]] constexpr Basis transposed() const noexcept {
        return {
            {x.x, y.x, z.x},
            {x.y, y.y, z.y},
            {x.z, y.z, z.z}
        };
    }

    [[nodiscard]] bool try_inverse(Basis& result) const noexcept {
        const real_t det = determinant();
        if (std::abs(det) <= REAL_EPSILON) {
            return false;
        }

        const Vector3 row0 = y.cross(z) / det;
        const Vector3 row1 = z.cross(x) / det;
        const Vector3 row2 = x.cross(y) / det;

        result = {
            {row0.x, row1.x, row2.x},
            {row0.y, row1.y, row2.y},
            {row0.z, row1.z, row2.z}
        };

        return true;
    }

    [[nodiscard]] Basis orthonormalized() const noexcept {
        const Vector3 axis_x = x.normalized();
        const Vector3 axis_y = (y - axis_x * axis_x.dot(y)).normalized();
        const Vector3 axis_z = axis_x.cross(axis_y).normalized();

        return {axis_x, axis_y, axis_z};
    }

    friend constexpr Basis operator*(const Basis& left, const Basis& right) noexcept {
        return {
            left.xform(right.x),
            left.xform(right.y),
            left.xform(right.z)
        };
    }

    friend constexpr bool operator==(const Basis&, const Basis&) noexcept = default;
};

} // namespace nle
