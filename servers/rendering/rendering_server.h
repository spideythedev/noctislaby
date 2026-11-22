#pragma once

#include "rendering_device.h"

#include <memory>
#include <mutex>
#include <span>

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
    [[nodiscard]] RenderingDeviceInfo get_device_info() const;

    [[nodiscard]] ErrorCode create_buffer(
        const BufferDescription& description,
        RID& result
    );
    [[nodiscard]] ErrorCode update_buffer(
        RID buffer,
        std::size_t offset,
        std::span<const std::byte> data
    );
    [[nodiscard]] ErrorCode read_buffer(
        RID buffer,
        std::size_t offset,
        std::span<std::byte> destination
    ) const;
    [[nodiscard]] ErrorCode free_buffer(RID buffer);\n\n    [[nodiscard]] ErrorCode create_texture(\n        const TextureDescription& description,\n        RID& result\n    );\n    [[nodiscard]] ErrorCode update_texture(\n        RID texture,\n        std::size_t offset,\n        std::span<const std::byte> data\n    );\n    [[nodiscard]] ErrorCode read_texture(\n        RID texture,\n        std::size_t offset,\n        std::span<std::byte> destination\n    ) const;\n    [[nodiscard]] ErrorCode free_texture(RID texture);

private:
    mutable std::mutex mutex_;
    std::unique_ptr<RenderingDevice> device_;
};

} // namespace nle
