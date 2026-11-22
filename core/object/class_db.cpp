#include "class_db.h"

#include <algorithm>
#include <utility>

namespace nle {

ClassDB& ClassDB::instance() {
    static ClassDB database;
    return database;
}

ClassDB::ClassDB() {
    classes_.emplace(
        StringName("Object"),
        ClassInfo{StringName("Object"), StringName{}, [] {
            return std::make_unique<Object>();
        }}
    );
}

bool ClassDB::register_class(
    StringName name,
    StringName parent,
    Factory factory
) {
    if (name.empty() || parent.empty() || name == parent) {
        return false;
    }

    std::lock_guard lock(mutex_);

    if (classes_.contains(name) || !classes_.contains(parent)) {
        return false;
    }

    const StringName key = name;
    ClassInfo info{std::move(name), std::move(parent), std::move(factory)};
    classes_.emplace(key, std::move(info));

    return true;
}

bool ClassDB::class_exists(const StringName& name) const {
    std::lock_guard lock(mutex_);
    return classes_.contains(name);
}

std::optional<ClassDB::ClassInfo> ClassDB::get_class_info(
    const StringName& name
) const {
    std::lock_guard lock(mutex_);

    const auto found = classes_.find(name);
    if (found == classes_.end()) {
        return std::nullopt;
    }

    return found->second;
}

std::unique_ptr<Object> ClassDB::instantiate(const StringName& name) const {
    Factory factory;

    {
        std::lock_guard lock(mutex_);

        const auto found = classes_.find(name);
        if (found == classes_.end() || !found->second.factory) {
            return nullptr;
        }

        factory = found->second.factory;
    }

    return factory();
}

bool ClassDB::is_parent_class(
    const StringName& child,
    const StringName& parent
) const {
    if (child == parent) {
        return false;
    }

    std::lock_guard lock(mutex_);

    auto found = classes_.find(child);
    while (found != classes_.end() && !found->second.parent.empty()) {
        if (found->second.parent == parent) {
            return true;
        }

        found = classes_.find(found->second.parent);
    }

    return false;
}

std::vector<StringName> ClassDB::get_class_list() const {
    std::vector<StringName> result;

    std::lock_guard lock(mutex_);
    result.reserve(classes_.size());

    for (const auto& [name, info] : classes_) {
        static_cast<void>(info);
        result.push_back(name);
    }

    std::sort(result.begin(), result.end());
    return result;
}

} // namespace nle
