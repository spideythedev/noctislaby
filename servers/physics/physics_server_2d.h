#pragma once

#include "core/error/error.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "servers/rendering/rid.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace nle {

enum class PhysicsBodyMode2D : std::uint8_t {
    Static,
    Dynamic
};

struct PhysicsBodyDescription2D final {
    PhysicsBodyMode2D mode = PhysicsBodyMode2D::Static;
    Rect2 shape{{0, 0}, {1, 1}};
    Vector2 position;
    Vector2 velocity;
    real_t mass = 1;
    real_t restitution = 0;
    std::uint32_t collision_layer = 1;
    std::uint32_t collision_mask = 0xffffffffU;
};

struct PhysicsBodyState2D final {
    Vector2 position;
    Vector2 velocity;
    Rect2 world_bounds;
};

class PhysicsServer2D final {
public:
    [[nodiscard]] ErrorCode create_body(
        const PhysicsBodyDescription2D& description,
        RID& result
    );
    [[nodiscard]] ErrorCode free_body(RID body);
    [[nodiscard]] ErrorCode set_body_position(RID body, Vector2 position);
    [[nodiscard]] ErrorCode set_body_velocity(RID body, Vector2 velocity);
    [[nodiscard]] ErrorCode set_body_collision_filter(
        RID body,
        std::uint32_t layer,
        std::uint32_t mask
    );
    [[nodiscard]] ErrorCode get_body_state(
        RID body,
        PhysicsBodyState2D& state
    ) const;
    [[nodiscard]] ErrorCode step(double delta, Vector2 gravity);
    [[nodiscard]] std::size_t get_body_count() const noexcept;

private:
    struct Body final {
        PhysicsBodyMode2D mode = PhysicsBodyMode2D::Static;
        Rect2 shape;
        Vector2 position;
        Vector2 velocity;
        real_t inverse_mass = 0;
        real_t restitution = 0;
        std::uint32_t collision_layer = 1;
        std::uint32_t collision_mask = 0xffffffffU;
    };

    struct Slot final {
        Body body;
        std::uint32_t generation = 1;
        bool occupied = false;
    };

    [[nodiscard]] Slot* get_slot(RID body) noexcept;
    [[nodiscard]] const Slot* get_slot(RID body) const noexcept;
    [[nodiscard]] static Rect2 get_world_bounds(const Body& body) noexcept;
    static void resolve_collision(Body& first, Body& second) noexcept;

    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    std::size_t body_count_ = 0;
};

} // namespace nle
