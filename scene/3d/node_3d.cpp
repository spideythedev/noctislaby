#include "node_3d.h"

#include <utility>

namespace nle {

Node3D::Node3D(std::string name)
    : Node(std::move(name)) {}

const char* Node3D::get_class_name() const noexcept {
    return "Node3D";
}

Transform3D Node3D::get_global_transform() const noexcept {
    const auto* parent_3d = dynamic_cast<const Node3D*>(get_parent());
    return parent_3d == nullptr
        ? transform_
        : parent_3d->get_global_transform() * transform_;
}

const Transform3D& Node3D::get_transform() const noexcept {
    return transform_;
}

void Node3D::set_transform(const Transform3D& transform) noexcept {
    transform_ = transform;
}

Vector3 Node3D::get_position() const noexcept {
    return transform_.origin;
}

void Node3D::set_position(const Vector3 position) noexcept {
    transform_.origin = position;
}

} // namespace nle
