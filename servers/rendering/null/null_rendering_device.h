#pragma once

#include "servers/rendering/rendering_device.h"

#include <mutex>
#include <vector>

namespace nle {

class NullRenderingDevice final : public RenderingDevice {
public:
    [[nodiscard]] ErrorCode initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] bool is_initialized() const noexcept override;
    [[nodiscard]] RenderingDeviceInfo get_device_info() const noexcept override;

    [[nodiscard]] ErrorCode create_buffer(
        const BufferDescription& description,
        RID& result
    ) override;
    [[nodiscard]] ErrorCode update_buffer(
        RID buffer,
        std::size_t offset,
        std::span<const std::byte> data
    ) override;
    [[nodiscard]] ErrorCode read_buffer(
        RID buffer,
        std::size_t offset,
        std::span<std::byte> destination
    ) const override;
    [[nodiscard]] ErrorCode free_buffer(RID buffer) override;

    [[nodiscard]] std::size_t get_buffer_count() const noexcept;

private:
    struct Buffer final {
        std::vector<std::byte> bytes;
        BufferUsage usage = BufferUsage::None;
    };

    struct Slot final {
        Buffer buffer;
        std::uint32_t generation = 1;
        bool occupied = false;
    };

    [[nodiscard]] Slot* get_slot(RID rid) noexcept;
    [[nodiscard]] const Slot* get_slot(RID rid) const noexcept;

    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    std::size_t buffer_count_ = 0;
    bool initialized_ = false;
};

} // namespace nle
