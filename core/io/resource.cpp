#include "resource.h"

#include <utility>

namespace nle {

const char* Resource::get_class_name() const noexcept {
    return "Resource";
}

ResourceUID Resource::get_uid() const noexcept {
    return uid_;
}

void Resource::set_uid(const ResourceUID uid) {
    if (uid_ == uid) {
        return;
    }

    uid_ = uid;
    emit_changed();
}

std::string_view Resource::get_path() const noexcept {
    return path_;
}

void Resource::set_path(std::string path) {
    if (path_ == path) {
        return;
    }

    path_ = std::move(path);
    emit_changed();
}

std::string_view Resource::get_name() const noexcept {
    return name_;
}

void Resource::set_name(std::string name) {
    if (name_ == name) {
        return;
    }

    name_ = std::move(name);
    emit_changed();
}

bool Resource::is_local_to_scene() const noexcept {
    return local_to_scene_;
}

void Resource::set_local_to_scene(const bool enabled) noexcept {
    local_to_scene_ = enabled;
}

Signal<>& Resource::changed_signal() noexcept {
    return changed_;
}

void Resource::emit_changed() {
    changed_.emit();
}

} // namespace nle
