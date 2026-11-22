#pragma once

#include "math/vector2.h"
#include "object/signal.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nle {

enum class InputEventType : std::uint8_t {
    Key,
    MouseButton,
    MouseMotion,
    JoypadButton,
    JoypadAxis
};

struct InputEvent final {
    InputEventType type = InputEventType::Key;
    std::int32_t device = 0;
    std::int32_t code = 0;
    bool pressed = false;
    bool echo = false;
    std::uint32_t modifiers = 0;
    real_t value = 0;
    Vector2 position;
    Vector2 relative;
};

struct InputControl final {
    InputEventType type = InputEventType::Key;
    std::int32_t device = 0;
    std::int32_t code = 0;

    friend constexpr bool operator==(const InputControl&, const InputControl&) noexcept = default;
};

struct InputControlHash final {
    [[nodiscard]] std::size_t operator()(const InputControl& control) const noexcept;
};

class InputServer final {
public:
    static constexpr std::size_t MaxQueuedEvents = 65'536;

    [[nodiscard]] bool push_event(const InputEvent& event);
    void process_frame();

    [[nodiscard]] bool is_pressed(InputControl control) const;
    [[nodiscard]] bool is_just_pressed(InputControl control) const;
    [[nodiscard]] bool is_just_released(InputControl control) const;
    [[nodiscard]] Vector2 get_mouse_position(std::int32_t device = 0) const;
    [[nodiscard]] std::size_t get_queued_event_count() const;

    [[nodiscard]] Signal<const InputEvent&>& event_signal() noexcept;

private:
    [[nodiscard]] static bool is_valid_event(const InputEvent& event) noexcept;
    [[nodiscard]] static bool has_button_state(InputEventType type) noexcept;

    mutable std::mutex mutex_;
    std::vector<InputEvent> queue_;
    std::unordered_set<InputControl, InputControlHash> pressed_;
    std::unordered_set<InputControl, InputControlHash> just_pressed_;
    std::unordered_set<InputControl, InputControlHash> just_released_;
    std::unordered_map<std::int32_t, Vector2> mouse_positions_;
    Signal<const InputEvent&> event_;
};

} // namespace nle
