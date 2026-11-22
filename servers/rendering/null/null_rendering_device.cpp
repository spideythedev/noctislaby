#include "null_rendering_device.h"

#include <algorithm>
#include <limits>
#include <new>

namespace nle {

ErrorCode NullRenderingDevice::initialize() {
    std::lock_guard lock(mutex_);
    if (initialized_) {
        return ErrorCode::AlreadyExists;
    }

    initialized_ = true;
    return ErrorCode::Ok;
}

void NullRenderingDevice::shutdown() noexcept {
    std::lock_guard lock(mutex_);
    slots_.clear();
    free_slots_.clear();
    buffer_count_ = 0;
    initialized_ = false;
}

bool NullRenderingDevice::is_initialized() const noexcept {
    std::lock_guard lock(mutex_);
    return initialized_;
}

RenderingDeviceInfo NullRenderingDevice::get_device_info() const noexcept {
    return {"NoctisLaby Null Device", 1, 0, false};
}

ErrorCode NullRenderingDevice::create_buffer(
    const BufferDescription& description,
    RID& result
) {
    result = {};
    if (description.size == 0 || description.initial_data.size() > description.size) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    std::uint32_t index = 0;
    if (free_slots_.empty()) {
        if (slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
            return ErrorCode::OutOfMemory;
        }

        try {
            free_slots_.reserve(slots_.size() + 1);
            slots_.emplace_back();
        } catch (const std::bad_alloc&) {
            return ErrorCode::OutOfMemory;
        }

        index = static_cast<std::uint32_t>(slots_.size() - 1);
    } else {
        index = free_slots_.back();
        free_slots_.pop_back();
    }

    Slot& slot = slots_[index];
    try {
        slot.buffer.bytes.assign(description.size, std::byte{0});
    } catch (const std::bad_alloc&) {
        free_slots_.push_back(index);
        return ErrorCode::OutOfMemory;
    }

    std::copy(
        description.initial_data.begin(),
        description.initial_data.end(),
        slot.buffer.bytes.begin()
    );
    slot.buffer.usage = description.usage;
    slot.occupied = true;
    ++buffer_count_;

    result = RID::from_parts(index, slot.generation);
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::update_buffer(
    const RID buffer,
    const std::size_t offset,
    const std::span<const std::byte> data
) {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    Slot* slot = get_slot(buffer);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    if (offset > slot->buffer.bytes.size() ||
        data.size() > slot->buffer.bytes.size() - offset) {
        return ErrorCode::InvalidParameter;
    }

    std::copy(data.begin(), data.end(), slot->buffer.bytes.begin() +
              static_cast<std::ptrdiff_t>(offset));
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::read_buffer(
    const RID buffer,
    const std::size_t offset,
    const std::span<std::byte> destination
) const {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    const Slot* slot = get_slot(buffer);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    if (offset > slot->buffer.bytes.size() ||
        destination.size() > slot->buffer.bytes.size() - offset) {
        return ErrorCode::InvalidParameter;
    }

    std::copy(
        slot->buffer.bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        slot->buffer.bytes.begin() +
            static_cast<std::ptrdiff_t>(offset + destination.size()),
        destination.begin()
    );
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::free_buffer(const RID buffer) {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    Slot* slot = get_slot(buffer);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    slot->buffer = {};
    slot->occupied = false;
    --buffer_count_;

    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot->generation;
        free_slots_.push_back(buffer.get_index());
    }

    return ErrorCode::Ok;
}

std::size_t NullRenderingDevice::get_buffer_count() const noexcept {
    std::lock_guard lock(mutex_);
    return buffer_count_;
}

NullRenderingDevice::Slot* NullRenderingDevice::get_slot(const RID rid) noexcept {
    if (!rid.is_valid() || rid.get_index() >= slots_.size()) {
        return nullptr;
    }

    Slot& slot = slots_[rid.get_index()];
    return slot.occupied && slot.generation == rid.get_generation() ? &slot : nullptr;
}

const NullRenderingDevice::Slot* NullRenderingDevice::get_slot(const RID rid) const noexcept {
    if (!rid.is_valid() || rid.get_index() >= slots_.size()) {
        return nullptr;
    }

    const Slot& slot = slots_[rid.get_index()];
    return slot.occupied && slot.generation == rid.get_generation() ? &slot : nullptr;
}

} // namespace nle
