#include "input_map.h"

#include <algorithm>
#include <utility>

namespace nle {

bool InputMap::add_action(const std::string_view name) {
    if (name.empty() || name.find_first_of("\r\n") != std::string_view::npos ||
        name.find('\0') != std::string_view::npos) {
        return false;
    }

    std::lock_guard lock(mutex_);
    return actions_.try_emplace(std::string(name)).second;
}

bool InputMap::remove_action(const std::string_view name) {
    std::lock_guard lock(mutex_);
    return actions_.erase(std::string(name)) != 0;
}

bool InputMap::has_action(const std::string_view name) const {
    std::lock_guard lock(mutex_);
    return actions_.contains(std::string(name));
}

bool InputMap::add_binding(
    const std::string_view action,
    const InputControl control
) {
    if (!is_valid_binding(control)) {
        return false;
    }

    std::lock_guard lock(mutex_);
    const auto found = actions_.find(std::string(action));
    if (found == actions_.end()) {
        return false;
    }

    const auto duplicate = std::find(found->second.begin(), found->second.end(), control);
    if (duplicate != found->second.end()) {
        return false;
    }

    found->second.push_back(control);
    return true;
}

bool InputMap::remove_binding(
    const std::string_view action,
    const InputControl control
) {
    std::lock_guard lock(mutex_);
    const auto found = actions_.find(std::string(action));
    if (found == actions_.end()) {
        return false;
    }

    const auto binding = std::find(found->second.begin(), found->second.end(), control);
    if (binding == found->second.end()) {
        return false;
    }

    found->second.erase(binding);
    return true;
}

std::vector<InputControl> InputMap::get_bindings(const std::string_view action) const {
    std::lock_guard lock(mutex_);
    const auto found = actions_.find(std::string(action));
    return found == actions_.end() ? std::vector<InputControl>{} : found->second;
}

real_t InputMap::get_action_strength(
    const InputServer& input,
    const std::string_view action
) const {
    const std::vector<InputControl> bindings = get_bindings(action);
    for (const InputControl control : bindings) {
        if (input.is_pressed(control)) {
            return real_t{1};
        }
    }
    return real_t{0};
}

bool InputMap::is_action_pressed(
    const InputServer& input,
    const std::string_view action
) const {
    return get_action_strength(input, action) > real_t{0};
}

bool InputMap::is_action_just_pressed(
    const InputServer& input,
    const std::string_view action
) const {
    const std::vector<InputControl> bindings = get_bindings(action);
    return std::any_of(bindings.begin(), bindings.end(), [&input](const InputControl control) {
        return input.is_just_pressed(control);
    });
}

bool InputMap::is_action_just_released(
    const InputServer& input,
    const std::string_view action
) const {
    const std::vector<InputControl> bindings = get_bindings(action);
    return std::any_of(bindings.begin(), bindings.end(), [&input](const InputControl control) {
        return input.is_just_released(control);
    });
}

bool InputMap::is_valid_binding(const InputControl control) noexcept {
    if (control.device < 0 || control.code < 0) {
        return false;
    }

    return control.type == InputEventType::Key ||
           control.type == InputEventType::MouseButton ||
           control.type == InputEventType::JoypadButton;
}

} // namespace nle
