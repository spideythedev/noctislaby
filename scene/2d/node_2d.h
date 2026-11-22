#pragma once

#include "math/transform2d.h"
#include "scene/main/node.h"

namespace nle {

class Node2D : public Node {
public:
    explicit Node2D(std::string name = "Node2D");

    [[nodiscard]] const char* get_class_name() const noexcept override;

    [[nodiscard]] const Transform2D& get_transform() const noexcept;
    [[nodiscard]] Transform2D get_global_transform() const noexcept;
    void set_transform(const Transform2D& transform) noexcept;

    [[nodiscard]] Vector2 get_position() const noexcept;
    void set_position(Vector2 position) noexcept;

private:
    Transform2D transform_;
};

} // namespace nle
