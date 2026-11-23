#pragma once

#include "node.h"

#include <limits>

namespace nle {

class Timer final : public Node {
public:
    enum class ProcessCallback {
        Idle,
        Physics
    };

    explicit Timer(std::string name = "Timer");

    [[nodiscard]] const char* get_class_name() const noexcept override;

    [[nodiscard]] bool set_wait_time(double seconds) noexcept;
    [[nodiscard]] double get_wait_time() const noexcept;
    [[nodiscard]] double get_time_left() const noexcept;

    void set_one_shot(bool enabled) noexcept;
    [[nodiscard]] bool is_one_shot() const noexcept;
    void set_autostart(bool enabled) noexcept;
    [[nodiscard]] bool has_autostart() const noexcept;
    void set_process_callback(ProcessCallback callback) noexcept;
    [[nodiscard]] ProcessCallback get_process_callback() const noexcept;

    [[nodiscard]] bool start(double time_seconds = -1.0) noexcept;
    void stop() noexcept;
    [[nodiscard]] bool is_stopped() const noexcept;

    [[nodiscard]] Signal<>& timeout_signal() noexcept;

protected:
    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;

private:
    void tick(double delta);

    double wait_time_ = 1.0;
    double time_left_ = 0.0;
    bool running_ = false;
    bool one_shot_ = false;
    bool autostart_ = false;
    ProcessCallback process_callback_ = ProcessCallback::Idle;
    Signal<> timeout_;
};

} // namespace nle
