#pragma once

#include "rendering_device.h"

#include <memory>
#include <mutex>

namespace nle {

class RenderingServer final {
public:
    RenderingServer() = default;
    ~RenderingServer();

    RenderingServer(const RenderingServer&) = delete;
    RenderingServer& operator=(const RenderingServer&) = delete;

    [[nodiscard]] ErrorCode initialize(std::unique_ptr<RenderingDevice> device);
    void shutdown() noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] RenderingDeviceInfo get_device_info() const noexcept;
    [[nodiscard]] RenderingDevice* get_device() noexcept;
    [[nodiscard]] const RenderingDevice* get_device() const noexcept;

private:
    mutable std::mutex mutex_;
    std::unique_ptr<RenderingDevice> device_;
};

} // namespace nle
