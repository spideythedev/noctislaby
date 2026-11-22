#include "node_2d.h"

#include <utility>

namespace nle {

Node2D::Node2D(std::string name)
    : Node(std::move(name)) {}

const char* Node2D::get_class_name() const noexcept {
    return "Node2D";
}

const Transform2D& Node2D::get_transform() const noexcept {
    return transform_;
}

void Node2D::set_transform(const Transform2D& transform) noexcept {
    transform_ = transform;
}

Vector2 Node2D::get_position() const noexcept {
    return transform_.origin;
}

void Node2D::set_position(const Vector2 position) noexcept {
    transform_.origin = position;
}

} // namespace nle
