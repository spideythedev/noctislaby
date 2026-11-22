#include "null_rendering_device.h"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

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
    texture_slots_.clear();
    free_texture_slots_.clear();
    texture_count_ = 0;
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
        } catch (const std::length_error&) {
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
    } catch (const std::length_error&) {
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


std::size_t NullRenderingDevice::get_texture_count() const noexcept {
    std::lock_guard lock(mutex_);
    return texture_count_;
}

ErrorCode NullRenderingDevice::create_texture(
    const TextureDescription& description,
    RID& result
) {
    result = {};
    const std::size_t bytes_per_pixel = texture_format_bytes_per_pixel(description.format);
    if (description.width == 0 || description.height == 0 || bytes_per_pixel == 0) {
        return ErrorCode::InvalidParameter;
    }

    const std::uint64_t pixel_count =
        static_cast<std::uint64_t>(description.width) * description.height;
    if (pixel_count > std::numeric_limits<std::size_t>::max() / bytes_per_pixel) {
        return ErrorCode::InvalidParameter;
    }
    const std::size_t byte_count = static_cast<std::size_t>(pixel_count) * bytes_per_pixel;
    if (description.initial_data.size() > byte_count) {
        return ErrorCode::InvalidParameter;
    }

    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    std::uint32_t index = 0;
    if (free_texture_slots_.empty()) {
        if (texture_slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
            return ErrorCode::OutOfMemory;
        }
        try {
            free_texture_slots_.reserve(texture_slots_.size() + 1);
            texture_slots_.emplace_back();
        } catch (const std::bad_alloc&) {
            return ErrorCode::OutOfMemory;
        } catch (const std::length_error&) {
            return ErrorCode::OutOfMemory;
        }
        index = static_cast<std::uint32_t>(texture_slots_.size() - 1);
    } else {
        index = free_texture_slots_.back();
        free_texture_slots_.pop_back();
    }

    TextureSlot& slot = texture_slots_[index];
    try {
        slot.texture.bytes.assign(byte_count, std::byte{0});
    } catch (const std::bad_alloc&) {
        free_texture_slots_.push_back(index);
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        free_texture_slots_.push_back(index);
        return ErrorCode::OutOfMemory;
    }

    std::copy(
        description.initial_data.begin(),
        description.initial_data.end(),
        slot.texture.bytes.begin()
    );
    slot.texture.width = description.width;
    slot.texture.height = description.height;
    slot.texture.format = description.format;
    slot.texture.usage = description.usage;
    slot.occupied = true;
    ++texture_count_;
    result = RID::from_parts(index, slot.generation);
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::update_texture(
    const RID texture,
    const std::size_t offset,
    const std::span<const std::byte> data
) {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    TextureSlot* slot = get_texture_slot(texture);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }
    if (offset > slot->texture.bytes.size() ||
        data.size() > slot->texture.bytes.size() - offset) {
        return ErrorCode::InvalidParameter;
    }

    std::copy(data.begin(), data.end(), slot->texture.bytes.begin() +
              static_cast<std::ptrdiff_t>(offset));
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::read_texture(
    const RID texture,
    const std::size_t offset,
    const std::span<std::byte> destination
) const {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    const TextureSlot* slot = get_texture_slot(texture);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }
    if (offset > slot->texture.bytes.size() ||
        destination.size() > slot->texture.bytes.size() - offset) {
        return ErrorCode::InvalidParameter;
    }

    std::copy(
        slot->texture.bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        slot->texture.bytes.begin() +
            static_cast<std::ptrdiff_t>(offset + destination.size()),
        destination.begin()
    );
    return ErrorCode::Ok;
}

ErrorCode NullRenderingDevice::free_texture(const RID texture) {
    std::lock_guard lock(mutex_);
    if (!initialized_) {
        return ErrorCode::InvalidState;
    }

    TextureSlot* slot = get_texture_slot(texture);
    if (slot == nullptr) {
        return ErrorCode::NotFound;
    }

    slot->texture = {};
    slot->occupied = false;
    --texture_count_;
    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        ++slot->generation;
        free_texture_slots_.push_back(texture.get_index());
    }
    return ErrorCode::Ok;
}

NullRenderingDevice::TextureSlot* NullRenderingDevice::get_texture_slot(
    const RID rid
) noexcept {
    if (!rid.is_valid() || rid.get_index() >= texture_slots_.size()) {
        return nullptr;
    }
    TextureSlot& slot = texture_slots_[rid.get_index()];
    return slot.occupied && slot.generation == rid.get_generation() ? &slot : nullptr;
}

const NullRenderingDevice::TextureSlot* NullRenderingDevice::get_texture_slot(
    const RID rid
) const noexcept {
    if (!rid.is_valid() || rid.get_index() >= texture_slots_.size()) {
        return nullptr;
    }
    const TextureSlot& slot = texture_slots_[rid.get_index()];
    return slot.occupied && slot.generation == rid.get_generation() ? &slot : nullptr;
}

} // namespace nle
