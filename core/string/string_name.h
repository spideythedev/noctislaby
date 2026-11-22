#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nle {

class StringName final {
public:
    StringName() noexcept = default;
    explicit StringName(std::string_view value);

    [[nodiscard]] std::string_view view() const noexcept;
    [[nodiscard]] std::string str() const;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t hash() const noexcept;

    friend bool operator==(const StringName& left, const StringName& right) noexcept;
    friend std::strong_ordering operator<=>(
        const StringName& left,
        const StringName& right
    ) noexcept;

private:
    std::shared_ptr<const std::string> value_;
};

} // namespace nle

namespace std {

template <>
struct hash<nle::StringName> {
    [[nodiscard]] std::size_t operator()(const nle::StringName& value) const noexcept {
        return value.hash();
    }
};

} // namespace std
