#pragma once

#include "resource_uid.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nle {

class ResourceUIDRegistry final {
public:
    [[nodiscard]] bool register_uid(ResourceUID uid, std::string_view path);
    [[nodiscard]] bool unregister_uid(ResourceUID uid);
    [[nodiscard]] ResourceUID get_uid(std::string_view path) const;
    [[nodiscard]] std::string get_path(ResourceUID uid) const;
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] bool save(const std::filesystem::path& file_path) const;
    [[nodiscard]] bool load(const std::filesystem::path& file_path);

    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, std::string> paths_by_uid_;
    std::unordered_map<std::string, std::uint64_t> uids_by_path_;
};

} // namespace nle
