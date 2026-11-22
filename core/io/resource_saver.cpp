#include "resource_saver.h"
#include "resource_path.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace nle {
namespace {

std::string get_extension(const std::string_view path) {
    const std::size_t separator = path.find_last_of("/\\");
    const std::size_t dot = path.find_last_of('.');

    if (dot == std::string_view::npos ||
        (separator != std::string_view::npos && dot < separator)) {
        return {};
    }

    std::string extension(path.substr(dot + 1));
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );

    return extension;
}

bool extension_matches(
    const std::vector<std::string>& extensions,
    const std::string_view extension
) {
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [extension](const std::string& candidate) {
            if (candidate.size() != extension.size()) {
                return false;
            }

            return std::equal(
                candidate.begin(),
                candidate.end(),
                extension.begin(),
                [](const char left, const char right) {
                    return std::tolower(static_cast<unsigned char>(left)) ==
                           std::tolower(static_cast<unsigned char>(right));
                }
            );
        }
    );
}

} // namespace

bool ResourceSaver::add_format_saver(std::shared_ptr<ResourceFormatSaver> saver) {
    if (saver == nullptr) {
        return false;
    }

    std::lock_guard lock(savers_mutex_);

    const auto found = std::find(savers_.begin(), savers_.end(), saver);
    if (found != savers_.end()) {
        return false;
    }

    savers_.push_back(std::move(saver));
    return true;
}

ErrorCode ResourceSaver::save(
    const std::string_view path,
    const Ref<Resource>& resource
) {
    std::string normalized_path;
    if (!resource || ResourcePath::normalize(path, normalized_path) != ErrorCode::Ok) {
        return ErrorCode::InvalidParameter;
    }

    const std::shared_ptr<ResourceFormatSaver> saver = find_saver(get_extension(normalized_path));
    if (saver == nullptr) {
        return ErrorCode::NotFound;
    }

    return saver->save(normalized_path, resource);
}

std::shared_ptr<ResourceFormatSaver> ResourceSaver::find_saver(
    const std::string_view extension
) const {
    std::vector<std::shared_ptr<ResourceFormatSaver>> savers;
    {
        std::lock_guard lock(savers_mutex_);
        savers = savers_;
    }

    for (const auto& saver : savers) {
        if (extension_matches(saver->get_recognized_extensions(), extension)) {
            return saver;
        }
    }

    return {};
}

} // namespace nle
