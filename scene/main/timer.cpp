#include "timer.h"

#include <cmath>

namespace nle {

Timer::Timer(std::string name) : Node(std::move(name)) {}

const char* Timer::get_class_name() const noexcept {
    return "Timer";
}

bool Timer::set_wait_time(const double seconds) noexcept {
    if (!std::isfinite(seconds) || seconds <= 0.0) {
        return false;
    }

    wait_time_ = seconds;
    return true;
}

double Timer::get_wait_time() const noexcept {
    return wait_time_;
}

double Timer::get_time_left() const noexcept {
    return running_ ? time_left_ : 0.0;
}

void Timer::set_one_shot(const bool enabled) noexcept {
    one_shot_ = enabled;
}

bool Timer::is_one_shot() const noexcept {
    return one_shot_;
}

void Timer::set_autostart(const bool enabled) noexcept {
    autostart_ = enabled;
}

bool Timer::has_autostart() const noexcept {
    return autostart_;
}

void Timer::set_process_callback(const ProcessCallback callback) noexcept {
    if (callback == ProcessCallback::Idle || callback == ProcessCallback::Physics) {
        process_callback_ = callback;
    }
}

Timer::ProcessCallback Timer::get_process_callback() const noexcept {
    return process_callback_;
}

bool Timer::start(const double time_seconds) noexcept {
    const double duration = time_seconds < 0.0 ? wait_time_ : time_seconds;
    if (!std::isfinite(duration) || duration <= 0.0) {
        return false;
    }

    time_left_ = duration;
    running_ = true;
    return true;
}

void Timer::stop() noexcept {
    running_ = false;
    time_left_ = 0.0;
}

bool Timer::is_stopped() const noexcept {
    return !running_;
}

Signal<>& Timer::timeout_signal() noexcept {
    return timeout_;
}

void Timer::_ready() {
    if (autostart_) {
        static_cast<void>(start());
    }
}

void Timer::_process(const double delta) {
    if (process_callback_ == ProcessCallback::Idle) {
        tick(delta);
    }
}

void Timer::_physics_process(const double delta) {
    if (process_callback_ == ProcessCallback::Physics) {
        tick(delta);
    }
}

void Timer::tick(const double delta) noexcept {
    if (!running_ || !std::isfinite(delta) || delta < 0.0) {
        return;
    }

    if (delta < time_left_) {
        time_left_ -= delta;
        return;
    }

    if (one_shot_) {
        stop();
    } else {
        time_left_ = wait_time_;
    }

    timeout_.emit();
}

} // namespace nle
