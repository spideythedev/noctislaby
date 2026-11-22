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

    [[nodiscard]] std::size_t get_buffer_count() const noexcept;\n    [[nodiscard]] std::size_t get_texture_count() const noexcept;\n\n    [[nodiscard]] ErrorCode create_texture(\n        const TextureDescription& description,\n        RID& result\n    ) override;\n    [[nodiscard]] ErrorCode update_texture(\n        RID texture,\n        std::size_t offset,\n        std::span<const std::byte> data\n    ) override;\n    [[nodiscard]] ErrorCode read_texture(\n        RID texture,\n        std::size_t offset,\n        std::span<std::byte> destination\n    ) const override;\n    [[nodiscard]] ErrorCode free_texture(RID texture) override;

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

    struct Texture final {\n        std::vector<std::byte> bytes;\n        TextureDescription description;\n    };\n\n    struct TextureSlot final {\n        Texture texture;\n        std::uint32_t generation = 1;\n        bool occupied = false;\n    };\n\n    [[nodiscard]] TextureSlot* get_texture_slot(RID rid) noexcept;\n    [[nodiscard]] const TextureSlot* get_texture_slot(RID rid) const noexcept;\n\n    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    std::size_t buffer_count_ = 0;\n    std::vector<TextureSlot> texture_slots_;\n    std::vector<std::uint32_t> free_texture_slots_;\n    std::size_t texture_count_ = 0;
    bool initialized_ = false;
};

} // namespace nle
