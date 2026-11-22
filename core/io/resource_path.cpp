#include "resource_path.h"

#include <cctype>
#include <string>
#include <vector>

namespace nle {

ErrorCode ResourcePath::normalize(
    const std::string_view path,
    std::string& normalized
) {
    normalized.clear();
    if (path.empty() || path.find('\0') != std::string_view::npos) {
        return ErrorCode::InvalidParameter;
    }

    const std::size_t scheme_end = path.find("://");
    if (scheme_end == std::string_view::npos || scheme_end == 0) {
        return ErrorCode::InvalidParameter;
    }

    std::string scheme(path.substr(0, scheme_end));
    for (char& character : scheme) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (!std::isalnum(value) && character != '+' && character != '-' && character != '.') {
            return ErrorCode::InvalidParameter;
        }
        character = static_cast<char>(std::tolower(value));
    }

    if (scheme != "res" && scheme != "user") {
        return ErrorCode::InvalidParameter;
    }

    const std::string_view relative = path.substr(scheme_end + 3);
    if (relative.empty()) {
        return ErrorCode::InvalidParameter;
    }

    std::vector<std::string_view> segments;
    std::size_t offset = 0;
    while (offset <= relative.size()) {
        const std::size_t separator = relative.find_first_of("/\\", offset);
        const std::size_t end = separator == std::string_view::npos
            ? relative.size()
            : separator;
        const std::string_view segment = relative.substr(offset, end - offset);

        if (segment.empty() || segment == ".") {
            // Repeated separators and current-directory segments are normalized away.
        } else if (segment == "..") {
            if (segments.empty()) {
                return ErrorCode::InvalidParameter;
            }
            segments.pop_back();
        } else {
            if (segment.find(':') != std::string_view::npos) {
                return ErrorCode::InvalidParameter;
            }
            segments.push_back(segment);
        }

        if (separator == std::string_view::npos) {
            break;
        }
        offset = separator + 1;
    }

    if (segments.empty()) {
        return ErrorCode::InvalidParameter;
    }

    normalized = scheme + "://";
    for (std::size_t index = 0; index < segments.size(); ++index) {
        if (index != 0) {
            normalized.push_back('/');
        }
        normalized.append(segments[index]);
    }

    return ErrorCode::Ok;
}

} // namespace nle
