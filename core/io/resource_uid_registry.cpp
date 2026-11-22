#include "resource_uid_registry.h"

#include "file_access.h"
#include "resource_path.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace nle {
namespace {

constexpr std::array<std::byte, 8> FileMagic{
    std::byte{'N'}, std::byte{'L'}, std::byte{'E'}, std::byte{'U'},
    std::byte{'I'}, std::byte{'D'}, std::byte{'0'}, std::byte{'1'}
};
constexpr std::uint32_t FileVersion = 1;
constexpr std::uint64_t MaxFileSize = 64ULL * 1024ULL * 1024ULL;
constexpr std::uint32_t MaxEntries = 1'000'000;
constexpr std::uint32_t MaxPathLength = 16'384;

void append_u32(std::vector<std::byte>& output, const std::uint32_t value) {
    for (unsigned int shift = 0; shift < 32; shift += 8) {
        output.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_u64(std::vector<std::byte>& output, const std::uint64_t value) {
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        output.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

bool read_u32(std::span<const std::byte> data, std::size_t& offset, std::uint32_t& value) {
    if (offset > data.size() || data.size() - offset < 4) {
        return false;
    }

    value = 0;
    for (unsigned int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(
            std::to_integer<unsigned char>(data[offset++])
        ) << shift;
    }
    return true;
}

bool read_u64(std::span<const std::byte> data, std::size_t& offset, std::uint64_t& value) {
    if (offset > data.size() || data.size() - offset < 8) {
        return false;
    }

    value = 0;
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(
            std::to_integer<unsigned char>(data[offset++])
        ) << shift;
    }
    return true;
}

std::uint64_t hash_bytes(const std::span<const std::byte> data) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const std::byte value : data) {
        hash ^= std::to_integer<unsigned char>(value);
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace

bool ResourceUIDRegistry::register_uid(const ResourceUID uid, const std::string_view path) {
    std::string owned_path;
    if (!uid.is_valid() || path.size() > MaxPathLength ||
        ResourcePath::normalize(path, owned_path) != ErrorCode::Ok) {
        return false;
    }
    std::lock_guard lock(mutex_);

    const auto uid_found = paths_by_uid_.find(uid.value);
    if (uid_found != paths_by_uid_.end()) {
        return uid_found->second == owned_path;
    }

    const auto path_found = uids_by_path_.find(owned_path);
    if (path_found != uids_by_path_.end()) {
        return path_found->second == uid.value;
    }

    paths_by_uid_.emplace(uid.value, owned_path);
    uids_by_path_.emplace(std::move(owned_path), uid.value);
    return true;
}

bool ResourceUIDRegistry::unregister_uid(const ResourceUID uid) {
    if (!uid.is_valid()) {
        return false;
    }

    std::lock_guard lock(mutex_);
    const auto found = paths_by_uid_.find(uid.value);
    if (found == paths_by_uid_.end()) {
        return false;
    }

    uids_by_path_.erase(found->second);
    paths_by_uid_.erase(found);
    return true;
}

ResourceUID ResourceUIDRegistry::get_uid(const std::string_view path) const {
    std::string normalized;
    if (ResourcePath::normalize(path, normalized) != ErrorCode::Ok) {
        return {};
    }

    std::lock_guard lock(mutex_);
    const auto found = uids_by_path_.find(normalized);
    return found == uids_by_path_.end() ? ResourceUID{} : ResourceUID{found->second};
}

std::string ResourceUIDRegistry::get_path(const ResourceUID uid) const {
    std::lock_guard lock(mutex_);
    const auto found = paths_by_uid_.find(uid.value);
    return found == paths_by_uid_.end() ? std::string{} : found->second;
}

std::size_t ResourceUIDRegistry::size() const {
    std::lock_guard lock(mutex_);
    return paths_by_uid_.size();
}

bool ResourceUIDRegistry::save(const std::filesystem::path& file_path) const {
    std::vector<std::pair<std::uint64_t, std::string>> entries;
    {
        std::lock_guard lock(mutex_);
        if (paths_by_uid_.size() > MaxEntries) {
            return false;
        }
        entries.reserve(paths_by_uid_.size());
        for (const auto& entry : paths_by_uid_) {
            entries.push_back(entry);
        }
    }

    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });

    std::vector<std::byte> data;
    data.insert(data.end(), FileMagic.begin(), FileMagic.end());
    append_u32(data, FileVersion);
    append_u32(data, static_cast<std::uint32_t>(entries.size()));

    for (const auto& [uid, path] : entries) {
        if (path.empty() || path.size() > MaxPathLength) {
            return false;
        }
        append_u64(data, uid);
        append_u32(data, static_cast<std::uint32_t>(path.size()));
        for (const unsigned char character : path) {
            data.push_back(static_cast<std::byte>(character));
        }
    }

    append_u64(data, hash_bytes(data));
    if (data.size() > MaxFileSize) {
        return false;
    }

    ErrorCode error = ErrorCode::Ok;
    auto file = FileAccess::open(file_path, FileMode::Write, &error);
    if (!file || !file->write(data)) {
        return false;
    }
    file->flush();
    return file->get_error() == ErrorCode::Ok;
}

bool ResourceUIDRegistry::load(const std::filesystem::path& file_path) {
    ErrorCode error = ErrorCode::Ok;
    auto file = FileAccess::open(file_path, FileMode::Read, &error);
    if (!file) {
        return false;
    }

    const std::uint64_t file_size = file->get_length();
    if (file_size < 24 || file_size > MaxFileSize ||
        file_size > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    std::vector<std::byte> data(static_cast<std::size_t>(file_size));
    if (file->read(data) != data.size()) {
        return false;
    }

    const std::span<const std::byte> bytes(data);
    if (!std::equal(FileMagic.begin(), FileMagic.end(), bytes.begin())) {
        return false;
    }

    const std::size_t checksum_offset = data.size() - 8;
    std::size_t checksum_cursor = checksum_offset;
    std::uint64_t stored_checksum = 0;
    if (!read_u64(bytes, checksum_cursor, stored_checksum) ||
        hash_bytes(bytes.first(checksum_offset)) != stored_checksum) {
        return false;
    }

    std::size_t offset = FileMagic.size();
    std::uint32_t version = 0;
    std::uint32_t count = 0;
    if (!read_u32(bytes.first(checksum_offset), offset, version) ||
        version != FileVersion ||
        !read_u32(bytes.first(checksum_offset), offset, count) ||
        count > MaxEntries) {
        return false;
    }

    std::unordered_map<std::uint64_t, std::string> paths_by_uid;
    std::unordered_map<std::string, std::uint64_t> uids_by_path;
    paths_by_uid.reserve(count);
    uids_by_path.reserve(count);

    for (std::uint32_t index = 0; index < count; ++index) {
        std::uint64_t uid = 0;
        std::uint32_t path_length = 0;
        if (!read_u64(bytes.first(checksum_offset), offset, uid) ||
            !read_u32(bytes.first(checksum_offset), offset, path_length) ||
            uid == 0 || path_length == 0 || path_length > MaxPathLength ||
            offset > checksum_offset || checksum_offset - offset < path_length) {
            return false;
        }

        std::string path;
        path.reserve(path_length);
        for (std::uint32_t character = 0; character < path_length; ++character) {
            path.push_back(static_cast<char>(
                std::to_integer<unsigned char>(bytes[offset++])
            ));
        }

        std::string normalized_path;
        if (ResourcePath::normalize(path, normalized_path) != ErrorCode::Ok ||
            normalized_path != path ||
            !paths_by_uid.emplace(uid, path).second ||
            !uids_by_path.emplace(std::move(path), uid).second) {
            return false;
        }
    }

    if (offset != checksum_offset) {
        return false;
    }

    {
        std::lock_guard lock(mutex_);
        paths_by_uid_.swap(paths_by_uid);
        uids_by_path_.swap(uids_by_path);
    }
    return true;
}

void ResourceUIDRegistry::clear() {
    std::lock_guard lock(mutex_);
    paths_by_uid_.clear();
    uids_by_path_.clear();
}

} // namespace nle
