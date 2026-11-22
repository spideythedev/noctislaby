#pragma once

#include "math/transform3d.h"
#include "scene/main/node.h"

namespace nle {

class Node3D : public Node {
public:
    explicit Node3D(std::string name = "Node3D");

    [[nodiscard]] const char* get_class_name() const noexcept override;

    [[nodiscard]] const Transform3D& get_transform() const noexcept;
    [[nodiscard]] Transform3D get_global_transform() const noexcept;
    [[nodiscard]] Vector3 get_global_position() const noexcept;
    [[nodiscard]] bool set_global_position(Vector3 position) noexcept;
    void set_transform(const Transform3D& transform) noexcept;

    [[nodiscard]] Vector3 get_position() const noexcept;
    void set_position(Vector3 position) noexcept;

private:
    Transform3D transform_;
};

} // namespace nle
