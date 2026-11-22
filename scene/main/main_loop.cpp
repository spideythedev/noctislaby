#include "main_loop.h"

#include <algorithm>
#include <cmath>

namespace nle {

MainLoop::MainLoop(SceneTree& scene_tree) noexcept
    : scene_tree_(scene_tree) {}

bool MainLoop::iteration(const double frame_delta, MainLoopFrameStats* stats) {
    if (!scene_tree_.is_initialized() || !std::isfinite(frame_delta) || frame_delta < 0.0) {
        return false;
    }

    const double clamped_delta = std::min(frame_delta, max_frame_delta_);
    const double scaled_delta = clamped_delta * time_scale_;
    const double physics_delta = 1.0 / static_cast<double>(physics_ticks_per_second_);

    physics_accumulator_ += scaled_delta;
    std::uint32_t physics_steps = 0;
    std::uint64_t dropped_steps = 0;

    while (physics_accumulator_ >= physics_delta &&
           physics_steps < max_physics_steps_per_frame_) {
        scene_tree_.physics_frame(physics_delta);
        physics_accumulator_ -= physics_delta;
        ++physics_steps;
    }

    if (physics_accumulator_ >= physics_delta) {
        dropped_steps = static_cast<std::uint64_t>(
            std::floor(physics_accumulator_ / physics_delta)
        );
        physics_accumulator_ = std::fmod(physics_accumulator_, physics_delta);
    }

    interpolation_alpha_ = std::clamp(physics_accumulator_ / physics_delta, 0.0, 1.0);
    scene_tree_.process_frame(scaled_delta);
    ++frame_index_;

    if (stats != nullptr) {
        *stats = MainLoopFrameStats{
            frame_index_,
            physics_steps,
            dropped_steps,
            scaled_delta,
            interpolation_alpha_
        };
    }

    return true;
}

bool MainLoop::set_physics_ticks_per_second(const std::uint32_t ticks_per_second) noexcept {
    if (ticks_per_second == 0 || ticks_per_second > 1000) {
        return false;
    }

    physics_ticks_per_second_ = ticks_per_second;
    physics_accumulator_ = 0.0;
    interpolation_alpha_ = 0.0;
    return true;
}

bool MainLoop::set_max_physics_steps_per_frame(const std::uint32_t steps) noexcept {
    if (steps == 0 || steps > 1024) {
        return false;
    }
    max_physics_steps_per_frame_ = steps;
    return true;
}

bool MainLoop::set_max_frame_delta(const double seconds) noexcept {
    if (!std::isfinite(seconds) || seconds <= 0.0 || seconds > 10.0) {
        return false;
    }
    max_frame_delta_ = seconds;
    return true;
}

bool MainLoop::set_time_scale(const double scale) noexcept {
    if (!std::isfinite(scale) || scale < 0.0 || scale > 100.0) {
        return false;
    }
    time_scale_ = scale;
    return true;
}

std::uint32_t MainLoop::get_physics_ticks_per_second() const noexcept {
    return physics_ticks_per_second_;
}

std::uint32_t MainLoop::get_max_physics_steps_per_frame() const noexcept {
    return max_physics_steps_per_frame_;
}

double MainLoop::get_max_frame_delta() const noexcept {
    return max_frame_delta_;
}

double MainLoop::get_time_scale() const noexcept {
    return time_scale_;
}

double MainLoop::get_interpolation_alpha() const noexcept {
    return interpolation_alpha_;
}

std::uint64_t MainLoop::get_frame_index() const noexcept {
    return frame_index_;
}

void MainLoop::reset() noexcept {
    physics_accumulator_ = 0.0;
    interpolation_alpha_ = 0.0;
    frame_index_ = 0;
}

} // namespace nle
