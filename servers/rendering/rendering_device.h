#pragma once

#include "core/error/error.h"
#include "rid.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace nle {

enum class BufferUsage : std::uint32_t {
    None = 0,
    Vertex = 1U << 0U,
    Index = 1U << 1U,
    Uniform = 1U << 2U,
    Storage = 1U << 3U,
    Readback = 1U << 4U
};

[[nodiscard]] constexpr BufferUsage operator|(
    const BufferUsage left,
    const BufferUsage right
) noexcept {
    return static_cast<BufferUsage>(
        static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right)
    );
}

[[nodiscard]] constexpr bool has_flag(
    const BufferUsage value,
    const BufferUsage flag
) noexcept {
    return (static_cast<std::uint32_t>(value) &
            static_cast<std::uint32_t>(flag)) != 0;
}

struct BufferDescription final {
    std::size_t size = 0;
    BufferUsage usage = BufferUsage::None;
    std::span<const std::byte> initial_data;
};

enum class TextureFormat : std::uint8_t {
    R8Unorm,
    RGBA8Unorm,
    BGRA8Unorm,
    RGBA16Float,
    Depth32Float
};

enum class TextureUsage : std::uint32_t {
    None = 0,
    Sampled = 1U << 0U,
    Storage = 1U << 1U,
    ColorAttachment = 1U << 2U,
    DepthAttachment = 1U << 3U,
    TransferSource = 1U << 4U,
    TransferDestination = 1U << 5U
};

[[nodiscard]] constexpr TextureUsage operator|(
    const TextureUsage left,
    const TextureUsage right
) noexcept {
    return static_cast<TextureUsage>(
        static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right)
    );
}

[[nodiscard]] constexpr bool has_flag(
    const TextureUsage value,
    const TextureUsage flag
) noexcept {
    return (static_cast<std::uint32_t>(value) &
            static_cast<std::uint32_t>(flag)) != 0;
}

[[nodiscard]] constexpr std::size_t texture_format_bytes_per_pixel(
    const TextureFormat format
) noexcept {
    switch (format) {
        case TextureFormat::R8Unorm: return 1;
        case TextureFormat::RGBA8Unorm:
        case TextureFormat::BGRA8Unorm:
        case TextureFormat::Depth32Float: return 4;
        case TextureFormat::RGBA16Float: return 8;
        default: return 0;
    }
}

struct TextureDescription final {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUsage usage = TextureUsage::None;
    std::span<const std::byte> initial_data;
};

struct RenderingDeviceInfo final {
    std::string name;
    std::uint32_t api_major = 0;
    std::uint32_t api_minor = 0;
    bool hardware_accelerated = false;
};

class RenderingDevice {
public:
    virtual ~RenderingDevice() = default;

    [[nodiscard]] virtual ErrorCode initialize() = 0;
    virtual void shutdown() noexcept = 0;
    [[nodiscard]] virtual bool is_initialized() const noexcept = 0;
    [[nodiscard]] virtual RenderingDeviceInfo get_device_info() const = 0;

    [[nodiscard]] virtual ErrorCode create_buffer(
        const BufferDescription& description,
        RID& result
    ) = 0;
    [[nodiscard]] virtual ErrorCode update_buffer(
        RID buffer,
        std::size_t offset,
        std::span<const std::byte> data
    ) = 0;
    [[nodiscard]] virtual ErrorCode read_buffer(
        RID buffer,
        std::size_t offset,
        std::span<std::byte> destination
    ) const = 0;
    [[nodiscard]] virtual ErrorCode free_buffer(RID buffer) = 0;

    [[nodiscard]] virtual ErrorCode create_texture(
        const TextureDescription& description,
        RID& result
    ) = 0;
    [[nodiscard]] virtual ErrorCode update_texture(
        RID texture,
        std::size_t offset,
        std::span<const std::byte> data
    ) = 0;
    [[nodiscard]] virtual ErrorCode read_texture(
        RID texture,
        std::size_t offset,
        std::span<std::byte> destination
    ) const = 0;
    [[nodiscard]] virtual ErrorCode free_texture(RID texture) = 0;
};

} // namespace nle
