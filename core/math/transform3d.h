#pragma once

#include "basis.h"

namespace nle {

struct Transform3D final {
    Basis basis;
    Vector3 origin;

    constexpr Transform3D() noexcept = default;
    constexpr Transform3D(const Basis basis_value, const Vector3 origin_value) noexcept
        : basis(basis_value), origin(origin_value) {}

    [[nodiscard]] constexpr Vector3 xform(const Vector3 point) const noexcept {
        return basis.xform(point) + origin;
    }

    [[nodiscard]] bool try_affine_inverse(Transform3D& result) const noexcept {
        Basis inverse_basis;
        if (!basis.try_inverse(inverse_basis)) {
            return false;
        }

        result = {inverse_basis, inverse_basis.xform(-origin)};
        return true;
    }

    friend constexpr Transform3D operator*(
        const Transform3D& left,
        const Transform3D& right
    ) noexcept {
        return {
            left.basis * right.basis,
            left.xform(right.origin)
        };
    }

    friend constexpr bool operator==(
        const Transform3D&,
        const Transform3D&
    ) noexcept = default;
};

} // namespace nle
