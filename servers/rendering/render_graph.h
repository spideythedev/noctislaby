#pragma once

#include "core/error/error.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nle {

using RenderGraphResourceId = std::uint32_t;
using RenderGraphPassId = std::uint32_t;

enum class RenderGraphAccess : std::uint8_t {
    Read,
    Write,
    ReadWrite
};

struct RenderGraphResourceUse final {
    RenderGraphResourceId resource = 0;
    RenderGraphAccess access = RenderGraphAccess::Read;
};

class RenderGraph final {
public:
    static constexpr RenderGraphResourceId InvalidResource = 0;
    static constexpr RenderGraphPassId InvalidPass = 0;

    [[nodiscard]] ErrorCode create_resource(
        std::string_view name,
        bool imported,
        RenderGraphResourceId& result
    );

    [[nodiscard]] ErrorCode add_pass(
        std::string_view name,
        std::span<const RenderGraphResourceUse> resources,
        RenderGraphPassId& result
    );

    // The dependent pass executes after the prerequisite pass.
    [[nodiscard]] ErrorCode add_dependency(
        RenderGraphPassId dependent,
        RenderGraphPassId prerequisite
    );

    // Produces a stable topological order. On failure, order is empty.
    [[nodiscard]] ErrorCode compile(std::vector<RenderGraphPassId>& order) const;

    [[nodiscard]] std::size_t get_resource_count() const noexcept;
    [[nodiscard]] std::size_t get_pass_count() const noexcept;
    void clear() noexcept;

private:
    struct Resource final {
        std::string name;
        bool imported = false;
    };

    struct Pass final {
        std::string name;
        std::vector<RenderGraphResourceUse> resources;
        std::vector<RenderGraphPassId> dependencies;
    };

    std::vector<Resource> resources_;
    std::vector<Pass> passes_;
};

} // namespace nle
