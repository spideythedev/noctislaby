#pragma once

#include "object.h"
#include "string/string_name.h"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nle {

class ClassDB final {
public:
    using Factory = std::function<std::unique_ptr<Object>()>;

    struct ClassInfo final {
        StringName name;
        StringName parent;
        Factory factory;

        [[nodiscard]] bool is_instantiable() const noexcept {
            return static_cast<bool>(factory);
        }
    };

    static ClassDB& instance();

    template <typename T>
    requires std::derived_from<T, Object> && std::default_initializable<T>
    bool register_class(
        const std::string_view name,
        const std::string_view parent = "Object"
    ) {
        Factory factory = [] {
            return std::make_unique<T>();
        };

        return register_class(StringName(name), StringName(parent), std::move(factory));
    }

    [[nodiscard]] bool class_exists(const StringName& name) const;
    [[nodiscard]] std::optional<ClassInfo> get_class_info(const StringName& name) const;
    [[nodiscard]] std::unique_ptr<Object> instantiate(const StringName& name) const;
    [[nodiscard]] bool is_parent_class(
        const StringName& child,
        const StringName& parent
    ) const;
    [[nodiscard]] std::vector<StringName> get_class_list() const;

private:
    ClassDB();

    bool register_class(StringName name, StringName parent, Factory factory);

    mutable std::mutex mutex_;
    std::unordered_map<StringName, ClassInfo> classes_;
};

} // namespace nle
