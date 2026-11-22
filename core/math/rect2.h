#pragma once

#include "vector2.h"

#include <algorithm>

namespace nle {

struct Rect2 final {
    Vector2 position;
    Vector2 size;

    constexpr Rect2() noexcept = default;
    constexpr Rect2(const Vector2 position_value, const Vector2 size_value) noexcept
        : position(position_value), size(size_value) {}

    [[nodiscard]] constexpr Vector2 end() const noexcept {
        return position + size;
    }

    [[nodiscard]] constexpr real_t area() const noexcept {
        return size.x * size.y;
    }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return size.x <= 0 || size.y <= 0;
    }

    [[nodiscard]] constexpr bool has_point(const Vector2 point) const noexcept {
        const Vector2 limit = end();
        return point.x >= position.x && point.y >= position.y &&
               point.x < limit.x && point.y < limit.y;
    }

    [[nodiscard]] constexpr bool intersects(const Rect2& other) const noexcept {
        const Vector2 limit = end();
        const Vector2 other_limit = other.end();

        return position.x < other_limit.x && limit.x > other.position.x &&
               position.y < other_limit.y && limit.y > other.position.y;
    }

    [[nodiscard]] Rect2 intersection(const Rect2& other) const noexcept {
        const Vector2 limit = end();
        const Vector2 other_limit = other.end();

        const Vector2 start{
            std::max(position.x, other.position.x),
            std::max(position.y, other.position.y)
        };
        const Vector2 finish{
            std::min(limit.x, other_limit.x),
            std::min(limit.y, other_limit.y)
        };

        if (finish.x <= start.x || finish.y <= start.y) {
            return {};
        }

        return {start, finish - start};
    }

    [[nodiscard]] Rect2 merge(const Rect2& other) const noexcept {
        const Vector2 start{
            std::min(position.x, other.position.x),
            std::min(position.y, other.position.y)
        };
        const Vector2 finish{
            std::max(end().x, other.end().x),
            std::max(end().y, other.end().y)
        };

        return {start, finish - start};
    }

    friend constexpr bool operator==(const Rect2&, const Rect2&) noexcept = default;
};

} // namespace nle
