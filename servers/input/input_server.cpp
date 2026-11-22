#include "input_server.h"

#include <cmath>
#include <functional>
#include <utility>

namespace nle {

std::size_t InputControlHash::operator()(const InputControl& control) const noexcept {
    std::size_t seed = std::hash<std::uint8_t>{}(
        static_cast<std::uint8_t>(control.type)
    );
    seed ^= std::hash<std::int32_t>{}(control.device) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<std::int32_t>{}(control.code) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    return seed;
}

bool InputServer::push_event(const InputEvent& event) {
    if (!is_valid_event(event)) {
        return false;
    }

    std::lock_guard lock(mutex_);
    if (queue_.size() >= MaxQueuedEvents) {
        return false;
    }

    queue_.push_back(event);
    return true;
}

void InputServer::process_frame() {
    std::vector<InputEvent> events;
    {
        std::lock_guard lock(mutex_);
        events.swap(queue_);
        just_pressed_.clear();
        just_released_.clear();

        for (const InputEvent& event : events) {
            if (event.type == InputEventType::MouseMotion) {
                mouse_positions_.insert_or_assign(event.device, event.position);
                continue;
            }

            if (!has_button_state(event.type)) {
                continue;
            }

            const InputControl control{event.type, event.device, event.code};
            if (event.pressed) {
                if (pressed_.insert(control).second && !event.echo) {
                    just_pressed_.insert(control);
                }
            } else if (pressed_.erase(control) != 0) {
                just_released_.insert(control);
            }
        }
    }

    for (const InputEvent& event : events) {
        event_.emit(event);
    }
}

bool InputServer::is_pressed(const InputControl control) const {
    std::lock_guard lock(mutex_);
    return pressed_.contains(control);
}

bool InputServer::is_just_pressed(const InputControl control) const {
    std::lock_guard lock(mutex_);
    return just_pressed_.contains(control);
}

bool InputServer::is_just_released(const InputControl control) const {
    std::lock_guard lock(mutex_);
    return just_released_.contains(control);
}

Vector2 InputServer::get_mouse_position(const std::int32_t device) const {
    std::lock_guard lock(mutex_);
    const auto found = mouse_positions_.find(device);
    return found == mouse_positions_.end() ? Vector2{} : found->second;
}

std::size_t InputServer::get_queued_event_count() const {
    std::lock_guard lock(mutex_);
    return queue_.size();
}

Signal<const InputEvent&>& InputServer::event_signal() noexcept {
    return event_;
}

bool InputServer::is_valid_event(const InputEvent& event) noexcept {
    if (event.device < 0) {
        return false;
    }

    switch (event.type) {
        case InputEventType::Key:
        case InputEventType::MouseButton:
        case InputEventType::JoypadButton:
            return event.code >= 0;
        case InputEventType::MouseMotion:
            return event.device >= 0 &&
                   std::isfinite(static_cast<double>(event.position.x)) &&
                   std::isfinite(static_cast<double>(event.position.y)) &&
                   std::isfinite(static_cast<double>(event.relative.x)) &&
                   std::isfinite(static_cast<double>(event.relative.y));
        case InputEventType::JoypadAxis:
            return event.device >= 0 && event.code >= 0 &&
                   std::isfinite(static_cast<double>(event.value)) &&
                   event.value >= real_t{-1} && event.value <= real_t{1};
        default:
            return false;
    }
}

bool InputServer::has_button_state(const InputEventType type) noexcept {
    return type == InputEventType::Key ||
           type == InputEventType::MouseButton ||
           type == InputEventType::JoypadButton;
}

} // namespace nle
