#pragma once

#include "error/error.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>

namespace nle {

enum class FileMode {
    Read,
    Write,
    ReadWrite,
    Append
};

class FileAccess final {
public:
    [[nodiscard]] static std::unique_ptr<FileAccess> open(
        const std::filesystem::path& path,
        FileMode mode,
        ErrorCode* error = nullptr
    );

    ~FileAccess();

    FileAccess(const FileAccess&) = delete;
    FileAccess& operator=(const FileAccess&) = delete;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] std::size_t read(std::span<std::byte> destination);
    [[nodiscard]] bool write(std::span<const std::byte> source);
    [[nodiscard]] bool seek(std::uint64_t position);
    [[nodiscard]] std::uint64_t get_position();
    [[nodiscard]] std::uint64_t get_length() const;
    [[nodiscard]] bool is_eof() const noexcept;
    [[nodiscard]] ErrorCode get_error() const noexcept;

    void flush();
    void close() noexcept;

private:
    FileAccess(std::filesystem::path path, FileMode mode);

    [[nodiscard]] std::ios::openmode get_open_mode() const noexcept;

    std::filesystem::path path_;
    FileMode mode_;
    std::fstream stream_;
    ErrorCode error_ = ErrorCode::Ok;
};

} // namespace nle
