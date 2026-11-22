#pragma once

#include "scene_tree.h"

#include <cstdint>

namespace nle {

struct MainLoopFrameStats final {
    std::uint64_t frame_index = 0;
    std::uint32_t physics_steps = 0;
    std::uint64_t dropped_physics_steps = 0;
    double frame_delta = 0.0;
    double interpolation_alpha = 0.0;
};

class MainLoop final {
public:
    explicit MainLoop(SceneTree& scene_tree) noexcept;

    [[nodiscard]] bool iteration(double frame_delta, MainLoopFrameStats* stats = nullptr);
    [[nodiscard]] bool set_physics_ticks_per_second(std::uint32_t ticks_per_second) noexcept;
    [[nodiscard]] bool set_max_physics_steps_per_frame(std::uint32_t steps) noexcept;
    [[nodiscard]] bool set_max_frame_delta(double seconds) noexcept;
    [[nodiscard]] bool set_time_scale(double scale) noexcept;

    [[nodiscard]] std::uint32_t get_physics_ticks_per_second() const noexcept;
    [[nodiscard]] std::uint32_t get_max_physics_steps_per_frame() const noexcept;
    [[nodiscard]] double get_max_frame_delta() const noexcept;
    [[nodiscard]] double get_time_scale() const noexcept;
    [[nodiscard]] double get_interpolation_alpha() const noexcept;
    [[nodiscard]] std::uint64_t get_frame_index() const noexcept;
    void reset() noexcept;

private:
    SceneTree& scene_tree_;
    std::uint32_t physics_ticks_per_second_ = 60;
    std::uint32_t max_physics_steps_per_frame_ = 8;
    double max_frame_delta_ = 0.25;
    double time_scale_ = 1.0;
    double physics_accumulator_ = 0.0;
    double interpolation_alpha_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

} // namespace nle
