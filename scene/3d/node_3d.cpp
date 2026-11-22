#include "node_3d.h"

#include <utility>

namespace nle {

Node3D::Node3D(std::string name)
    : Node(std::move(name)) {}

const char* Node3D::get_class_name() const noexcept {
    return "Node3D";
}

Transform3D Node3D::get_global_transform() const noexcept {
    const Node* ancestor = get_parent();
    while (ancestor != nullptr) {
        if (const auto* parent_3d = dynamic_cast<const Node3D*>(ancestor)) {
            return parent_3d->get_global_transform() * transform_;
        }
        ancestor = ancestor->get_parent();
    }

    return transform_;
}

Vector3 Node3D::get_global_position() const noexcept {
    return get_global_transform().origin;
}

bool Node3D::set_global_position(const Vector3 position) noexcept {
    const Node* ancestor = get_parent();
    while (ancestor != nullptr) {
        if (const auto* parent_3d = dynamic_cast<const Node3D*>(ancestor)) {
            Transform3D inverse;
            if (!parent_3d->get_global_transform().try_affine_inverse(inverse)) {
                return false;
            }

            transform_.origin = inverse.xform(position);
            return true;
        }
        ancestor = ancestor->get_parent();
    }

    transform_.origin = position;
    return true;
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
