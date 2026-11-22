#pragma once

#include "object.h"

#include <atomic>
#include <concepts>
#include <cstdint>
#include <exception>
#include <type_traits>
#include <utility>

namespace nle {

class RefCounted : public Object {
public:
    bool reference() noexcept {
        std::uint32_t count = reference_count_.load(std::memory_order_relaxed);

        while (count != UINT32_MAX) {
            if (reference_count_.compare_exchange_weak(
                    count, count + 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                return true;
            }
        }

        return false;
    }

    bool unreference() noexcept {
        const std::uint32_t previous =
            reference_count_.fetch_sub(1, std::memory_order_acq_rel);

        if (previous == 0) {
            reference_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        if (previous == 1) {
            delete this;
            return true;
        }

        return false;
    }

    [[nodiscard]] std::uint32_t get_reference_count() const noexcept {
        return reference_count_.load(std::memory_order_acquire);
    }

protected:
    RefCounted() = default;
    ~RefCounted() override = default;

private:
    std::atomic<std::uint32_t> reference_count_{0};
};

template <typename T>
requires std::derived_from<T, RefCounted>
class Ref final {
public:
    Ref() noexcept = default;

    explicit Ref(T* object) : object_(object) {
        retain();
    }

    Ref(const Ref& other) : object_(other.object_) {
        retain();
    }

    Ref(Ref&& other) noexcept : object_(std::exchange(other.object_, nullptr)) {}

    ~Ref() {
        release();
    }

    Ref& operator=(Ref other) noexcept {
        swap(other);
        return *this;
    }

    void reset() noexcept {
        release();
    }

    void swap(Ref& other) noexcept {
        std::swap(object_, other.object_);
    }

    [[nodiscard]] T* get() const noexcept {
        return object_;
    }

    [[nodiscard]] T* operator->() const noexcept {
        return object_;
    }

    [[nodiscard]] T& operator*() const noexcept {
        return *object_;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return object_ != nullptr;
    }

private:
    void retain() {
        if (object_ != nullptr && !object_->reference()) {
            object_ = nullptr;
            std::terminate();
        }
    }

    void release() noexcept {
        T* object = std::exchange(object_, nullptr);
        if (object != nullptr) {
            object->unreference();
        }
    }

    T* object_ = nullptr;
};

template <typename T, typename... Args>
requires std::derived_from<T, RefCounted>
[[nodiscard]] Ref<T> make_ref(Args&&... args) {
    return Ref<T>(new T(std::forward<Args>(args)...));
}

} // namespace nle
