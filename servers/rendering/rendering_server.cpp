#include "rendering_server.h"

#include <utility>

namespace nle {

RenderingServer::~RenderingServer() {
    shutdown();
}

ErrorCode RenderingServer::initialize(std::unique_ptr<RenderingDevice> device) {
    if (device == nullptr) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    if (device_ != nullptr) {
        return ErrorCode::AlreadyExists;
    }

    const ErrorCode result = device->initialize();
    if (result != ErrorCode::Ok) {
        return result;
    }

    device_ = std::move(device);
    return ErrorCode::Ok;
}

void RenderingServer::shutdown() noexcept {
    std::unique_ptr<RenderingDevice> device;
    {
        std::lock_guard lock(mutex_);
        device = std::move(device_);
    }

    if (device != nullptr) {
        device->shutdown();
    }
}

bool RenderingServer::is_initialized() const noexcept {
    std::lock_guard lock(mutex_);
    return device_ != nullptr && device_->is_initialized();
}

RenderingDeviceInfo RenderingServer::get_device_info() const noexcept {
    std::lock_guard lock(mutex_);
    return device_ == nullptr ? RenderingDeviceInfo{} : device_->get_device_info();
}

RenderingDevice* RenderingServer::get_device() noexcept {
    std::lock_guard lock(mutex_);
    return device_.get();
}

const RenderingDevice* RenderingServer::get_device() const noexcept {
    std::lock_guard lock(mutex_);
    return device_.get();
}

} // namespace nle
