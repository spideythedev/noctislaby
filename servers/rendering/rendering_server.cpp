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
    std::lock_guard lock(mutex_);
    if (device_ != nullptr) {
        device_->shutdown();
        device_.reset();
    }
}

bool RenderingServer::is_initialized() const noexcept {
    std::lock_guard lock(mutex_);
    return device_ != nullptr && device_->is_initialized();
}

RenderingDeviceInfo RenderingServer::get_device_info() const {
    std::lock_guard lock(mutex_);
    return device_ == nullptr ? RenderingDeviceInfo{} : device_->get_device_info();
}

ErrorCode RenderingServer::create_buffer(
    const BufferDescription& description,
    RID& result
) {
    std::lock_guard lock(mutex_);
    return device_ == nullptr
        ? ErrorCode::InvalidState
        : device_->create_buffer(description, result);
}

ErrorCode RenderingServer::update_buffer(
    const RID buffer,
    const std::size_t offset,
    const std::span<const std::byte> data
) {
    std::lock_guard lock(mutex_);
    return device_ == nullptr
        ? ErrorCode::InvalidState
        : device_->update_buffer(buffer, offset, data);
}

ErrorCode RenderingServer::read_buffer(
    const RID buffer,
    const std::size_t offset,
    const std::span<std::byte> destination
) const {
    std::lock_guard lock(mutex_);
    return device_ == nullptr
        ? ErrorCode::InvalidState
        : device_->read_buffer(buffer, offset, destination);
}

ErrorCode RenderingServer::free_buffer(const RID buffer) {
    std::lock_guard lock(mutex_);
    return device_ == nullptr
        ? ErrorCode::InvalidState
        : device_->free_buffer(buffer);
}

} // namespace nle
