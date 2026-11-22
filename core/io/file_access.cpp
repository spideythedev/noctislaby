#include "file_access.h"

#include <limits>
#include <utility>

namespace nle {

FileAccess::FileAccess(std::filesystem::path path, const FileMode mode)
    : path_(std::move(path)), mode_(mode) {}

std::unique_ptr<FileAccess> FileAccess::open(
    const std::filesystem::path& path,
    const FileMode mode,
    ErrorCode* error
) {
    auto file = std::unique_ptr<FileAccess>(new FileAccess(path, mode));
    file->stream_.open(path, file->get_open_mode());

    if (!file->stream_.is_open()) {
        file->error_ = ErrorCode::IoError;
        if (error != nullptr) {
            *error = file->error_;
        }
        return nullptr;
    }

    if (error != nullptr) {
        *error = ErrorCode::Ok;
    }

    return file;
}

FileAccess::~FileAccess() {
    close();
}

std::ios::openmode FileAccess::get_open_mode() const noexcept {
    switch (mode_) {
        case FileMode::Read:
            return std::ios::in | std::ios::binary;
        case FileMode::Write:
            return std::ios::out | std::ios::trunc | std::ios::binary;
        case FileMode::ReadWrite:
            return std::ios::in | std::ios::out | std::ios::binary;
        case FileMode::Append:
            return std::ios::out | std::ios::app | std::ios::binary;
    }

    return std::ios::binary;
}

bool FileAccess::is_open() const noexcept {
    return stream_.is_open();
}

std::size_t FileAccess::read(const std::span<std::byte> destination) {
    if (!is_open() || mode_ == FileMode::Write || mode_ == FileMode::Append) {
        error_ = ErrorCode::InvalidState;
        return 0;
    }

    if (destination.empty()) {
        return 0;
    }

    if (destination.size() >
        static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        error_ = ErrorCode::InvalidParameter;
        return 0;
    }

    stream_.read(
        reinterpret_cast<char*>(destination.data()),
        static_cast<std::streamsize>(destination.size())
    );

    const std::streamsize count = stream_.gcount();
    if (stream_.bad()) {
        error_ = ErrorCode::IoError;
    }

    return count > 0 ? static_cast<std::size_t>(count) : 0;
}

bool FileAccess::write(const std::span<const std::byte> source) {
    if (!is_open() || mode_ == FileMode::Read) {
        error_ = ErrorCode::InvalidState;
        return false;
    }

    if (source.empty()) {
        return true;
    }

    if (source.size() >
        static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        error_ = ErrorCode::InvalidParameter;
        return false;
    }

    stream_.write(
        reinterpret_cast<const char*>(source.data()),
        static_cast<std::streamsize>(source.size())
    );

    if (!stream_) {
        error_ = ErrorCode::IoError;
        return false;
    }

    return true;
}

bool FileAccess::seek(const std::uint64_t position) {
    if (!is_open() ||
        position > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        error_ = ErrorCode::InvalidParameter;
        return false;
    }

    stream_.clear();
    const auto offset = static_cast<std::streamoff>(position);

    if (mode_ != FileMode::Write && mode_ != FileMode::Append) {
        stream_.seekg(offset, std::ios::beg);
    }

    if (mode_ != FileMode::Read) {
        stream_.seekp(offset, std::ios::beg);
    }

    if (!stream_) {
        error_ = ErrorCode::IoError;
        return false;
    }

    return true;
}

std::uint64_t FileAccess::get_position() {
    if (!is_open()) {
        error_ = ErrorCode::InvalidState;
        return 0;
    }

    const std::streampos position =
        mode_ == FileMode::Read ? stream_.tellg() : stream_.tellp();

    if (position < 0) {
        error_ = ErrorCode::IoError;
        return 0;
    }

    return static_cast<std::uint64_t>(position);
}

std::uint64_t FileAccess::get_length() const {
    std::error_code error;
    const std::uintmax_t length = std::filesystem::file_size(path_, error);

    if (error || length > std::numeric_limits<std::uint64_t>::max()) {
        return 0;
    }

    return static_cast<std::uint64_t>(length);
}

bool FileAccess::is_eof() const noexcept {
    return stream_.eof();
}

ErrorCode FileAccess::get_error() const noexcept {
    return error_;
}

void FileAccess::flush() {
    if (is_open() && mode_ != FileMode::Read) {
        stream_.flush();
        if (!stream_) {
            error_ = ErrorCode::IoError;
        }
    }
}

void FileAccess::close() noexcept {
    if (stream_.is_open()) {
        stream_.close();
    }
}

} // namespace nle
