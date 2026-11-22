#pragma once

#include "input_server.h"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nle {

class InputMap final {
public:
    [[nodiscard]] bool add_action(std::string_view name);
    [[nodiscard]] bool remove_action(std::string_view name);
    [[nodiscard]] bool has_action(std::string_view name) const;

    [[nodiscard]] bool add_binding(std::string_view action, InputControl control);
    [[nodiscard]] bool remove_binding(std::string_view action, InputControl control);
    [[nodiscard]] std::vector<InputControl> get_bindings(std::string_view action) const;

    [[nodiscard]] real_t get_action_strength(
        const InputServer& input,
        std::string_view action
    ) const;
    [[nodiscard]] bool is_action_pressed(
        const InputServer& input,
        std::string_view action
    ) const;
    [[nodiscard]] bool is_action_just_pressed(
        const InputServer& input,
        std::string_view action
    ) const;
    [[nodiscard]] bool is_action_just_released(
        const InputServer& input,
        std::string_view action
    ) const;

private:
    [[nodiscard]] static bool is_valid_binding(InputControl control) noexcept;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<InputControl>> actions_;
};

} // namespace nle
