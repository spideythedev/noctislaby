#pragma once

#include "project_config.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace nle {

class Project final {
public:
    static constexpr std::string_view ProjectFileName = "project.nleproject";

    [[nodiscard]] static ErrorCode create(
        const std::filesystem::path& location,
        std::string_view name
    );
    [[nodiscard]] static ErrorCode load(
        const std::filesystem::path& location,
        Project& project
    );

    [[nodiscard]] ErrorCode save() const;
    [[nodiscard]] const std::string& get_name() const noexcept;
    [[nodiscard]] const std::filesystem::path& get_root_path() const noexcept;
    [[nodiscard]] const std::filesystem::path& get_file_path() const noexcept;
    [[nodiscard]] ProjectConfig& get_config() noexcept;
    [[nodiscard]] const ProjectConfig& get_config() const noexcept;

private:
    ProjectConfig config_;
    std::string name_;
    std::filesystem::path root_path_;
    std::filesystem::path file_path_;
};

} // namespace nle
