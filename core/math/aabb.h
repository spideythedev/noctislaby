#pragma once

#include "vector3.h"

#include <algorithm>

namespace nle {

struct AABB final {
    Vector3 position;
    Vector3 size;

    constexpr AABB() noexcept = default;
    constexpr AABB(const Vector3 position_value, const Vector3 size_value) noexcept
        : position(position_value), size(size_value) {}

    [[nodiscard]] constexpr Vector3 end() const noexcept {
        return position + size;
    }

    [[nodiscard]] constexpr real_t volume() const noexcept {
        return size.x * size.y * size.z;
    }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return size.x <= 0 || size.y <= 0 || size.z <= 0;
    }

    [[nodiscard]] constexpr bool has_point(const Vector3 point) const noexcept {
        const Vector3 limit = end();
        return point.x >= position.x && point.y >= position.y &&
               point.z >= position.z && point.x < limit.x &&
               point.y < limit.y && point.z < limit.z;
    }

    [[nodiscard]] constexpr bool intersects(const AABB& other) const noexcept {
        const Vector3 limit = end();
        const Vector3 other_limit = other.end();

        return position.x < other_limit.x && limit.x > other.position.x &&
               position.y < other_limit.y && limit.y > other.position.y &&
               position.z < other_limit.z && limit.z > other.position.z;
    }

    [[nodiscard]] AABB intersection(const AABB& other) const noexcept {
        const Vector3 limit = end();
        const Vector3 other_limit = other.end();

        const Vector3 start{
            std::max(position.x, other.position.x),
            std::max(position.y, other.position.y),
            std::max(position.z, other.position.z)
        };
        const Vector3 finish{
            std::min(limit.x, other_limit.x),
            std::min(limit.y, other_limit.y),
            std::min(limit.z, other_limit.z)
        };

        if (finish.x <= start.x || finish.y <= start.y || finish.z <= start.z) {
            return {};
        }

        return {start, finish - start};
    }

    [[nodiscard]] AABB merge(const AABB& other) const noexcept {
        const Vector3 start{
            std::min(position.x, other.position.x),
            std::min(position.y, other.position.y),
            std::min(position.z, other.position.z)
        };
        const Vector3 finish{
            std::max(end().x, other.end().x),
            std::max(end().y, other.end().y),
            std::max(end().z, other.end().z)
        };

        return {start, finish - start};
    }

    friend constexpr bool operator==(const AABB&, const AABB&) noexcept = default;
};

} // namespace nle
