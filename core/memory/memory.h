#pragma once

#include <cstddef>
#include <cstdint>

namespace nle {

struct MemoryStats final {
    std::uint64_t active_allocations = 0;
    std::uint64_t allocated_bytes = 0;
};

class Memory final {
public:
    [[nodiscard]] static void* allocate(
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t)
    );

    [[nodiscard]] static void* reallocate(
        void* memory,
        std::size_t old_size,
        std::size_t new_size,
        std::size_t alignment = alignof(std::max_align_t)
    );

    static void deallocate(
        void* memory,
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t)
    ) noexcept;

    [[nodiscard]] static MemoryStats get_stats() noexcept;

private:
    Memory() = delete;
};

} // namespace nle
