#pragma once

#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace nle {

class Variant final {
public:
    enum class Type : std::uint8_t {
        Nil,
        Bool,
        Int,
        Float,
        String,
        Vector2,
        Vector3,
        Vector4
    };

    Variant() noexcept = default;
    Variant(bool value);
    Variant(std::int32_t value);
    Variant(std::int64_t value);
    Variant(real_t value);
    Variant(const char* value);
    Variant(std::string value);
    Variant(std::string_view value);
    Variant(nle::Vector2 value);
    Variant(nle::Vector3 value);
    Variant(nle::Vector4 value);

    [[nodiscard]] Type get_type() const noexcept;
    [[nodiscard]] bool is_nil() const noexcept;
    [[nodiscard]] std::string to_string() const;

    template <typename T>
    [[nodiscard]] const T* get_if() const noexcept {
        return std::get_if<T>(&value_);
    }

    template <typename T>
    [[nodiscard]] T* get_if() noexcept {
        return std::get_if<T>(&value_);
    }

    friend bool operator==(const Variant&, const Variant&) = default;

private:
    using Storage = std::variant<
        std::monostate,
        bool,
        std::int64_t,
        real_t,
        std::string,
        nle::Vector2,
        nle::Vector3,
        nle::Vector4
    >;

    Storage value_;
};

} // namespace nle
