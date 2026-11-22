#pragma once

#include "object/ref_counted.h"
#include "object/signal.h"
#include "resource_uid.h"

#include <string>
#include <string_view>

namespace nle {

class Resource : public RefCounted {
public:
    Resource() = default;
    ~Resource() override = default;

    [[nodiscard]] const char* get_class_name() const noexcept override;

    [[nodiscard]] ResourceUID get_uid() const noexcept;
    void set_uid(ResourceUID uid);

    [[nodiscard]] std::string_view get_path() const noexcept;
    void set_path(std::string path);

    [[nodiscard]] std::string_view get_name() const noexcept;
    void set_name(std::string name);

    [[nodiscard]] bool is_local_to_scene() const noexcept;
    void set_local_to_scene(bool enabled) noexcept;

    [[nodiscard]] Signal<>& changed_signal() noexcept;
    void emit_changed();

private:
    ResourceUID uid_;
    std::string path_;
    std::string name_;
    bool local_to_scene_ = false;
    Signal<> changed_;
};

} // namespace nle
