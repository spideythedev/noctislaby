#include "memory.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstring>
#include <new>
#include <stdexcept>

namespace nle {
namespace {

std::atomic<std::uint64_t> active_allocations{0};
std::atomic<std::uint64_t> allocated_bytes{0};

std::size_t normalize_alignment(const std::size_t alignment) {
    if (!std::has_single_bit(alignment)) {
        throw std::invalid_argument("Memory alignment must be a non-zero power of two.");
    }

    return std::max(alignment, alignof(std::max_align_t));
}

} // namespace

void* Memory::allocate(const std::size_t size, const std::size_t alignment) {
    const std::size_t actual_alignment = normalize_alignment(alignment);
    const std::size_t actual_size = std::max<std::size_t>(size, 1);

    void* memory = ::operator new(actual_size, std::align_val_t(actual_alignment));

    active_allocations.fetch_add(1, std::memory_order_relaxed);
    allocated_bytes.fetch_add(size, std::memory_order_relaxed);

    return memory;
}

void* Memory::reallocate(
    void* memory,
    const std::size_t old_size,
    const std::size_t new_size,
    const std::size_t alignment
) {
    if (memory == nullptr) {
        return new_size == 0 ? nullptr : allocate(new_size, alignment);
    }

    if (new_size == 0) {
        deallocate(memory, old_size, alignment);
        return nullptr;
    }

    void* replacement = allocate(new_size, alignment);
    std::memcpy(replacement, memory, std::min(old_size, new_size));
    deallocate(memory, old_size, alignment);

    return replacement;
}

void Memory::deallocate(
    void* memory,
    const std::size_t size,
    const std::size_t alignment
) noexcept {
    if (memory == nullptr) {
        return;
    }

    const std::size_t actual_alignment = normalize_alignment(alignment);
    ::operator delete(memory, std::align_val_t(actual_alignment));

    active_allocations.fetch_sub(1, std::memory_order_relaxed);
    allocated_bytes.fetch_sub(size, std::memory_order_relaxed);
}

MemoryStats Memory::get_stats() noexcept {
    return {
        active_allocations.load(std::memory_order_relaxed),
        allocated_bytes.load(std::memory_order_relaxed)
    };
}

} // namespace nle
