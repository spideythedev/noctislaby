#pragma once

#include "vector2.h"

#include <cmath>

namespace nle {

struct Transform2D final {
    Vector2 x{1, 0};
    Vector2 y{0, 1};
    Vector2 origin;

    constexpr Transform2D() noexcept = default;
    constexpr Transform2D(
        const Vector2 x_axis,
        const Vector2 y_axis,
        const Vector2 origin_value
    ) noexcept : x(x_axis), y(y_axis), origin(origin_value) {}

    [[nodiscard]] constexpr Vector2 xform(const Vector2 point) const noexcept {
        return x * point.x + y * point.y + origin;
    }

    [[nodiscard]] constexpr real_t determinant() const noexcept {
        return x.cross(y);
    }

    [[nodiscard]] bool try_affine_inverse(Transform2D& result) const noexcept {
        const real_t det = determinant();
        if (std::abs(det) <= REAL_EPSILON) {
            return false;
        }

        const real_t inverse_det = real_t{1} / det;
        const Vector2 inverse_x{y.y * inverse_det, -x.y * inverse_det};
        const Vector2 inverse_y{-y.x * inverse_det, x.x * inverse_det};

        result = {
            inverse_x,
            inverse_y,
            {
                -(inverse_x.x * origin.x + inverse_y.x * origin.y),
                -(inverse_x.y * origin.x + inverse_y.y * origin.y)
            }
        };

        return true;
    }

    friend constexpr Transform2D operator*(
        const Transform2D& left,
        const Transform2D& right
    ) noexcept {
        return {
            {
                left.x.x * right.x.x + left.y.x * right.x.y,
                left.x.y * right.x.x + left.y.y * right.x.y
            },
            {
                left.x.x * right.y.x + left.y.x * right.y.y,
                left.x.y * right.y.x + left.y.y * right.y.y
            },
            left.xform(right.origin)
        };
    }

    friend constexpr bool operator==(
        const Transform2D&,
        const Transform2D&
    ) noexcept = default;
};

} // namespace nle
