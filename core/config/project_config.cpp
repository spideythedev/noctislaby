#include "project_config.h"

#include "io/file_access.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace nle {
namespace {

constexpr std::uint64_t MaxConfigSize = 4ULL * 1024ULL * 1024ULL;
constexpr std::size_t MaxLineLength = 65'536;

std::string_view trim(std::string_view value) noexcept {
    const std::size_t first = value.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

bool valid_identifier(const std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    for (const unsigned char character : value) {
        if (character < 0x20 || character == '[' || character == ']' ||
            character == '=' || character == '\n' || character == '\r') {
            return false;
        }
    }
    return true;
}

bool decode_value(const std::string_view source, std::string& value) {
    value.clear();
    if (source.empty() || source.front() != '"') {
        if (source.find('\0') != std::string_view::npos) {
            return false;
        }
        value.assign(source);
        return true;
    }

    if (source.size() < 2 || source.back() != '"') {
        return false;
    }

    for (std::size_t index = 1; index + 1 < source.size(); ++index) {
        char character = source[index];
        if (character == '\\') {
            if (++index + 1 >= source.size()) {
                return false;
            }
            switch (source[index]) {
                case '"': character = '"'; break;
                case '\\': character = '\\'; break;
                case 'n': character = '\n'; break;
                case 'r': character = '\r'; break;
                case 't': character = '\t'; break;
                default: return false;
            }
        } else if (character == '"') {
            return false;
        }
        value.push_back(character);
    }
    return value.find('\0') == std::string::npos;
}

std::string encode_value(const std::string_view value) {
    std::string encoded;
    encoded.reserve(value.size() + 2);
    encoded.push_back('"');
    for (const char character : value) {
        switch (character) {
            case '"': encoded.append("\\\""); break;
            case '\\': encoded.append("\\\\"); break;
            case '\n': encoded.append("\\n"); break;
            case '\r': encoded.append("\\r"); break;
            case '\t': encoded.append("\\t"); break;
            default: encoded.push_back(character); break;
        }
    }
    encoded.push_back('"');
    return encoded;
}

} // namespace

bool ProjectConfig::set_value(
    const std::string_view section,
    const std::string_view key,
    std::string value
) {
    if (!valid_identifier(section) || !valid_identifier(key) ||
        value.find('\0') != std::string::npos) {
        return false;
    }

    values_[std::string(section)].insert_or_assign(std::string(key), std::move(value));
    return true;
}

bool ProjectConfig::has_value(
    const std::string_view section,
    const std::string_view key
) const {
    const auto found_section = values_.find(section);
    return found_section != values_.end() && found_section->second.contains(key);
}

std::optional<std::string> ProjectConfig::get_value(
    const std::string_view section,
    const std::string_view key
) const {
    const auto found_section = values_.find(section);
    if (found_section == values_.end()) {
        return std::nullopt;
    }

    const auto found_value = found_section->second.find(key);
    if (found_value == found_section->second.end()) {
        return std::nullopt;
    }
    return found_value->second;
}

bool ProjectConfig::get_bool(
    const std::string_view section,
    const std::string_view key,
    bool& value
) const {
    const auto stored = get_value(section, key);
    if (!stored) {
        return false;
    }

    if (*stored == "true" || *stored == "1") {
        value = true;
        return true;
    }
    if (*stored == "false" || *stored == "0") {
        value = false;
        return true;
    }
    return false;
}

bool ProjectConfig::get_int(
    const std::string_view section,
    const std::string_view key,
    std::int64_t& value
) const {
    const auto stored = get_value(section, key);
    if (!stored || stored->empty()) {
        return false;
    }

    std::int64_t parsed = 0;
    const char* begin = stored->data();
    const char* end = begin + stored->size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    value = parsed;
    return true;
}

ErrorCode ProjectConfig::load(const std::filesystem::path& file_path) {
    ErrorCode error = ErrorCode::Ok;
    auto file = FileAccess::open(file_path, FileMode::Read, &error);
    if (!file) {
        return error;
    }

    const std::uint64_t length = file->get_length();
    if (length > MaxConfigSize || length > static_cast<std::uint64_t>(SIZE_MAX)) {
        return ErrorCode::InvalidParameter;
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(length));
    if (!bytes.empty() && file->read(bytes) != bytes.size()) {
        return ErrorCode::IoError;
    }

    std::string content;
    content.reserve(bytes.size());
    for (const std::byte byte : bytes) {
        const char character = static_cast<char>(std::to_integer<unsigned char>(byte));
        if (character == '\0') {
            return ErrorCode::InvalidParameter;
        }
        content.push_back(character);
    }

    std::map<std::string, Section, std::less<>> parsed;
    std::string current_section;
    std::size_t offset = 0;

    while (offset <= content.size()) {
        const std::size_t newline = content.find('\n', offset);
        const std::size_t end = newline == std::string::npos ? content.size() : newline;
        const std::string_view line = trim(std::string_view(content).substr(offset, end - offset));

        if (line.size() > MaxLineLength) {
            return ErrorCode::InvalidParameter;
        }

        if (!line.empty() && line.front() != '#' && line.front() != ';') {
            if (line.front() == '[') {
                if (line.size() < 3 || line.back() != ']') {
                    return ErrorCode::InvalidParameter;
                }
                const std::string_view section = trim(line.substr(1, line.size() - 2));
                if (!valid_identifier(section)) {
                    return ErrorCode::InvalidParameter;
                }
                current_section.assign(section);
                parsed.try_emplace(current_section);
            } else {
                const std::size_t equals = line.find('=');
                if (equals == std::string_view::npos || current_section.empty()) {
                    return ErrorCode::InvalidParameter;
                }

                const std::string_view key = trim(line.substr(0, equals));
                const std::string_view raw_value = trim(line.substr(equals + 1));
                std::string decoded;
                if (!valid_identifier(key) || !decode_value(raw_value, decoded)) {
                    return ErrorCode::InvalidParameter;
                }

                if (!parsed[current_section].emplace(std::string(key), std::move(decoded)).second) {
                    return ErrorCode::AlreadyExists;
                }
            }
        }

        if (newline == std::string::npos) {
            break;
        }
        offset = newline + 1;
    }

    values_.swap(parsed);
    return ErrorCode::Ok;
}

ErrorCode ProjectConfig::save(const std::filesystem::path& file_path) const {
    std::string content;
    for (const auto& [section, entries] : values_) {
        content.push_back('[');
        content.append(section);
        content.append("]\n");

        for (const auto& [key, value] : entries) {
            content.append(key);
            content.append(" = ");
            content.append(encode_value(value));
            content.push_back('\n');
        }
        content.push_back('\n');

        if (content.size() > MaxConfigSize) {
            return ErrorCode::InvalidParameter;
        }
    }

    const auto bytes = std::as_bytes(std::span(content.data(), content.size()));
    ErrorCode error = ErrorCode::Ok;
    auto file = FileAccess::open(file_path, FileMode::Write, &error);
    if (!file || !file->write(bytes)) {
        return error == ErrorCode::Ok ? ErrorCode::IoError : error;
    }

    file->flush();
    return file->get_error();
}

void ProjectConfig::clear() noexcept {
    values_.clear();
}

} // namespace nle
