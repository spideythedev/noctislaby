#pragma once

#include "error/error.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace nle {

class ProjectConfig final {
public:
    [[nodiscard]] bool set_value(
        std::string_view section,
        std::string_view key,
        std::string value
    );

    [[nodiscard]] bool has_value(std::string_view section, std::string_view key) const;
    [[nodiscard]] std::optional<std::string> get_value(
        std::string_view section,
        std::string_view key
    ) const;
    [[nodiscard]] bool get_bool(
        std::string_view section,
        std::string_view key,
        bool& value
    ) const;
    [[nodiscard]] bool get_int(
        std::string_view section,
        std::string_view key,
        std::int64_t& value
    ) const;

    [[nodiscard]] ErrorCode load(const std::filesystem::path& file_path);
    [[nodiscard]] ErrorCode save(const std::filesystem::path& file_path) const;
    void clear() noexcept;

private:
    using Section = std::map<std::string, std::string, std::less<>>;
    std::map<std::string, Section, std::less<>> values_;
};

} // namespace nle
