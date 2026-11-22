#include "project.h"

#include "io/resource_path.h"

#include <system_error>
#include <utility>

namespace nle {
namespace {

std::filesystem::path resolve_project_file(const std::filesystem::path& location) {
    if (location.extension() == ".nleproject") {
        return location;
    }
    return location / Project::ProjectFileName;
}

} // namespace

ErrorCode Project::create(
    const std::filesystem::path& location,
    const std::string_view name
) {
    if (location.empty() || name.empty()) {
        return ErrorCode::InvalidParameter;
    }

    const std::filesystem::path file_path = resolve_project_file(location);
    const std::filesystem::path root_path = file_path.parent_path().empty()
        ? std::filesystem::current_path()
        : file_path.parent_path();

    std::error_code filesystem_error;
    std::filesystem::create_directories(root_path, filesystem_error);
    if (filesystem_error) {
        return ErrorCode::IoError;
    }

    if (std::filesystem::exists(file_path, filesystem_error)) {
        return filesystem_error ? ErrorCode::IoError : ErrorCode::AlreadyExists;
    }
    if (filesystem_error) {
        return ErrorCode::IoError;
    }

    ProjectConfig config;
    if (!config.set_value("application", "name", std::string(name)) ||
        !config.set_value("application", "config_version", "1") ||
        !config.set_value("rendering", "renderer", "null") ||
        !config.set_value("physics", "ticks_per_second", "60")) {
        return ErrorCode::InvalidParameter;
    }

    return config.save(file_path);
}

ErrorCode Project::load(
    const std::filesystem::path& location,
    Project& project
) {
    if (location.empty()) {
        return ErrorCode::InvalidParameter;
    }

    const std::filesystem::path file_path = resolve_project_file(location);
    Project candidate;
    const ErrorCode load_error = candidate.config_.load(file_path);
    if (load_error != ErrorCode::Ok) {
        return load_error;
    }

    const auto name = candidate.config_.get_value("application", "name");
    if (!name || name->empty()) {
        return ErrorCode::InvalidParameter;
    }

    std::int64_t config_version = 0;
    if (!candidate.config_.get_int("application", "config_version", config_version) ||
        config_version != 1) {
        return ErrorCode::NotImplemented;
    }

    if (const auto main_scene = candidate.config_.get_value("application", "main_scene");
        main_scene && !main_scene->empty()) {
        std::string normalized;
        if (ResourcePath::normalize(*main_scene, normalized) != ErrorCode::Ok) {
            return ErrorCode::InvalidParameter;
        }
        if (normalized != *main_scene) {
            static_cast<void>(candidate.config_.set_value(
                "application", "main_scene", std::move(normalized)
            ));
        }
    }

    std::error_code filesystem_error;
    candidate.root_path_ = std::filesystem::absolute(
        file_path.parent_path().empty() ? std::filesystem::current_path() : file_path.parent_path(),
        filesystem_error
    ).lexically_normal();
    if (filesystem_error) {
        return ErrorCode::IoError;
    }

    candidate.file_path_ = std::filesystem::absolute(file_path, filesystem_error).lexically_normal();
    if (filesystem_error) {
        return ErrorCode::IoError;
    }

    candidate.name_ = *name;
    project = std::move(candidate);
    return ErrorCode::Ok;
}

ErrorCode Project::save() const {
    if (file_path_.empty()) {
        return ErrorCode::InvalidState;
    }
    return config_.save(file_path_);
}

const std::string& Project::get_name() const noexcept {
    return name_;
}

const std::filesystem::path& Project::get_root_path() const noexcept {
    return root_path_;
}

const std::filesystem::path& Project::get_file_path() const noexcept {
    return file_path_;
}

ProjectConfig& Project::get_config() noexcept {
    return config_;
}

const ProjectConfig& Project::get_config() const noexcept {
    return config_;
}

} // namespace nle
