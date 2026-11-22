#include "physics_server_2d.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>

namespace nle {
namespace {

bool is_finite(const Vector2 value) noexcept {
    return std::isfinite(static_cast<double>(value.x)) &&
           std::isfinite(static_cast<double>(value.y));
}

bool intersects(const Rect2& first, const Rect2& second) noexcept {
    return first.position.x < second.position.x + second.size.x &&
           first.position.x + first.size.x > second.position.x &&
           first.position.y < second.position.y + second.size.y &&
           first.position.y + first.size.y > second.position.y;
}

} // namespace

ErrorCode PhysicsServer2D::create_body(
    const PhysicsBodyDescription2D& description,
    RID& result
) {
    result = {};
    if (!is_finite(description.position) || !is_finite(description.velocity) ||
        !is_finite(description.shape.position) || !is_finite(description.shape.size) ||
        description.shape.size.x <= 0 || description.shape.size.y <= 0 ||
        !std::isfinite(static_cast<double>(description.mass)) ||
        !std::isfinite(static_cast<double>(description.restitution)) ||
        description.restitution < 0 || description.restitution > 1 ||
        (description.mode == PhysicsBodyMode2D::Dynamic && description.mass <= 0)) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    std::uint32_t index = 0;
    if (free_slots_.empty()) {
        if (slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
            return ErrorCode::OutOfMemory;
        }
        try {
            free_slots_.reserve(slots_.size() + 1);
            slots_.emplace_back();
        } catch (const std::bad_alloc&) {
            return ErrorCode::OutOfMemory;
        } catch (const std::length_error&) {
            return ErrorCode::OutOfMemory;
        }
        index = static_cast<std::uint32_t>(slots_.size() - 1);
    } else {
        index = free_slots_.back();
        free_slots_.pop_back();
    }

    Slot& slot = slots_[index];
    slot.body = Body{
        description.mode,
        description.shape,
        description.position,
        description.velocity,
        description.mode == PhysicsBodyMode2D::Dynamic
            ? real_t{1} / description.mass
            : real_t{0},
        description.restitution,
        description.collision_layer,
        description.collision_mask
    };
    slot.occupied = true;
    ++body_count_;
    result = RID::from_parts(index, slot.generation);
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::free_body(const RID body) {
    std::lock_guard lock(mutex_);
    Slot* slot = get_slot(body);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    slot->body = {};
    slot->occupied = false;
    --body_count_;
    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot->generation;
        free_slots_.push_back(body.get_index());
    }
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::set_body_position(const RID body, const Vector2 position) {
    if (!is_finite(position)) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    Slot* slot = get_slot(body);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }
    slot->body.position = position;
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::set_body_velocity(const RID body, const Vector2 velocity) {
    if (!is_finite(velocity)) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    Slot* slot = get_slot(body);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }
    slot->body.velocity = velocity;
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::set_body_collision_filter(
    const RID body,
    const std::uint32_t layer,
    const std::uint32_t mask
) {
    std::lock_guard lock(mutex_);
    Slot* slot = get_slot(body);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }
    slot->body.collision_layer = layer;
    slot->body.collision_mask = mask;
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::get_body_state(
    const RID body,
    PhysicsBodyState2D& state
) const {
    std::lock_guard lock(mutex_);
    const Slot* slot = get_slot(body);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    state.position = slot->body.position;
    state.velocity = slot->body.velocity;
    state.world_bounds = get_world_bounds(slot->body);
    return ErrorCode::Ok;
}

ErrorCode PhysicsServer2D::step(const double delta, const Vector2 gravity) {
    if (!std::isfinite(delta) || delta <= 0.0 || delta > 0.25 ||
        !is_finite(gravity)) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    const real_t step_delta = static_cast<real_t>(delta);
    for (Slot& slot : slots_) {
        if (!slot.occupied || slot.body.mode != PhysicsBodyMode2D::Dynamic) {
            continue;
        }

        slot.body.velocity.x += gravity.x * step_delta;
        slot.body.velocity.y += gravity.y * step_delta;
        slot.body.position.x += slot.body.velocity.x * step_delta;
        slot.body.position.y += slot.body.velocity.y * step_delta;
    }

    for (std::size_t first_index = 0; first_index < slots_.size(); ++first_index) {
        Slot& first_slot = slots_[first_index];
        if (!first_slot.occupied) {
            continue;
        }

        for (std::size_t second_index = first_index + 1;
             second_index < slots_.size();
             ++second_index) {
            Slot& second_slot = slots_[second_index];
            if (!second_slot.occupied ||
                (first_slot.body.inverse_mass == 0 && second_slot.body.inverse_mass == 0) ||
                (first_slot.body.collision_mask & second_slot.body.collision_layer) == 0 ||
                (second_slot.body.collision_mask & first_slot.body.collision_layer) == 0) {
                continue;
            }

            if (intersects(
                get_world_bounds(first_slot.body),
                get_world_bounds(second_slot.body)
            )) {
                resolve_collision(first_slot.body, second_slot.body);
            }
        }
    }

    return ErrorCode::Ok;
}

std::size_t PhysicsServer2D::get_body_count() const noexcept {
    std::lock_guard lock(mutex_);
    return body_count_;
}

PhysicsServer2D::Slot* PhysicsServer2D::get_slot(const RID body) noexcept {
    if (!body.is_valid() || body.get_index() >= slots_.size()) {
        return nullptr;
    }
    Slot& slot = slots_[body.get_index()];
    return slot.occupied && slot.generation == body.get_generation() ? &slot : nullptr;
}

const PhysicsServer2D::Slot* PhysicsServer2D::get_slot(const RID body) const noexcept {
    if (!body.is_valid() || body.get_index() >= slots_.size()) {
        return nullptr;
    }
    const Slot& slot = slots_[body.get_index()];
    return slot.occupied && slot.generation == body.get_generation() ? &slot : nullptr;
}

Rect2 PhysicsServer2D::get_world_bounds(const Body& body) noexcept {
    return {
        {body.position.x + body.shape.position.x, body.position.y + body.shape.position.y},
        body.shape.size
    };
}

void PhysicsServer2D::resolve_collision(Body& first, Body& second) noexcept {
    const Rect2 first_bounds = get_world_bounds(first);
    const Rect2 second_bounds = get_world_bounds(second);
    const real_t overlap_x = std::min(
        first_bounds.position.x + first_bounds.size.x,
        second_bounds.position.x + second_bounds.size.x
    ) - std::max(first_bounds.position.x, second_bounds.position.x);
    const real_t overlap_y = std::min(
        first_bounds.position.y + first_bounds.size.y,
        second_bounds.position.y + second_bounds.size.y
    ) - std::max(first_bounds.position.y, second_bounds.position.y);

    if (overlap_x <= 0 || overlap_y <= 0) {
        return;
    }

    Vector2 normal;
    real_t penetration = 0;
    const real_t first_center_x = first_bounds.position.x + first_bounds.size.x * real_t{0.5};
    const real_t second_center_x = second_bounds.position.x + second_bounds.size.x * real_t{0.5};
    const real_t first_center_y = first_bounds.position.y + first_bounds.size.y * real_t{0.5};
    const real_t second_center_y = second_bounds.position.y + second_bounds.size.y * real_t{0.5};

    if (overlap_x < overlap_y) {
        normal = {first_center_x <= second_center_x ? real_t{1} : real_t{-1}, 0};
        penetration = overlap_x;
    } else {
        normal = {0, first_center_y <= second_center_y ? real_t{1} : real_t{-1}};
        penetration = overlap_y;
    }

    const real_t inverse_mass_sum = first.inverse_mass + second.inverse_mass;
    if (inverse_mass_sum <= 0) {
        return;
    }

    constexpr real_t Slop = real_t{0.001};
    constexpr real_t CorrectionPercent = real_t{0.8};
    const real_t correction = std::max(penetration - Slop, real_t{0}) *
                              CorrectionPercent / inverse_mass_sum;
    first.position.x -= normal.x * correction * first.inverse_mass;
    first.position.y -= normal.y * correction * first.inverse_mass;
    second.position.x += normal.x * correction * second.inverse_mass;
    second.position.y += normal.y * correction * second.inverse_mass;

    const Vector2 relative_velocity{
        second.velocity.x - first.velocity.x,
        second.velocity.y - first.velocity.y
    };
    const real_t velocity_along_normal = relative_velocity.dot(normal);
    if (velocity_along_normal >= 0) {
        return;
    }

    const real_t restitution = std::min(first.restitution, second.restitution);
    const real_t impulse_magnitude =
        -(real_t{1} + restitution) * velocity_along_normal / inverse_mass_sum;
    const Vector2 impulse{
        normal.x * impulse_magnitude,
        normal.y * impulse_magnitude
    };

    first.velocity.x -= impulse.x * first.inverse_mass;
    first.velocity.y -= impulse.y * first.inverse_mass;
    second.velocity.x += impulse.x * second.inverse_mass;
    second.velocity.y += impulse.y * second.inverse_mass;
}

} // namespace nle
