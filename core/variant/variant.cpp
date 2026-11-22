#include "variant.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

namespace nle {

Variant::Variant(const bool value) : value_(value) {}
Variant::Variant(const std::int32_t value) : value_(static_cast<std::int64_t>(value)) {}
Variant::Variant(const std::int64_t value) : value_(value) {}
Variant::Variant(const real_t value) : value_(value) {}
Variant::Variant(const char* value) : value_(std::string(value == nullptr ? "" : value)) {}
Variant::Variant(std::string value) : value_(std::move(value)) {}
Variant::Variant(const std::string_view value) : value_(std::string(value)) {}
Variant::Variant(const nle::Vector2 value) : value_(value) {}
Variant::Variant(const nle::Vector3 value) : value_(value) {}
Variant::Variant(const nle::Vector4 value) : value_(value) {}

Variant::Type Variant::get_type() const noexcept {
    return static_cast<Type>(value_.index());
}

bool Variant::is_nil() const noexcept {
    return std::holds_alternative<std::monostate>(value_);
}

std::string Variant::to_string() const {
    return std::visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(value);
        } else if constexpr (std::is_floating_point_v<T>) {
            std::ostringstream stream;
            stream << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
            return stream.str();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else {
            std::ostringstream stream;
            stream << '(' << value.x << ", " << value.y;

            if constexpr (std::is_same_v<T, nle::Vector3> ||
                          std::is_same_v<T, nle::Vector4>) {
                stream << ", " << value.z;
            }

            if constexpr (std::is_same_v<T, nle::Vector4>) {
                stream << ", " << value.w;
            }

            stream << ')';
            return stream.str();
        }
    }, value_);
}

} // namespace nle
