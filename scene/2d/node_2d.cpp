#include "node_2d.h"

#include <utility>

namespace nle {

Node2D::Node2D(std::string name)
    : Node(std::move(name)) {}

const char* Node2D::get_class_name() const noexcept {
    return "Node2D";
}

Transform2D Node2D::get_global_transform() const noexcept {
    const Node* ancestor = get_parent();
    while (ancestor != nullptr) {
        if (const auto* parent_2d = dynamic_cast<const Node2D*>(ancestor)) {
            return parent_2d->get_global_transform() * transform_;
        }
        ancestor = ancestor->get_parent();
    }

    return transform_;
}

Vector2 Node2D::get_global_position() const noexcept {
    return get_global_transform().origin;
}

bool Node2D::set_global_position(const Vector2 position) noexcept {
    const Node* ancestor = get_parent();
    while (ancestor != nullptr) {
        if (const auto* parent_2d = dynamic_cast<const Node2D*>(ancestor)) {
            Transform2D inverse;
            if (!parent_2d->get_global_transform().try_affine_inverse(inverse)) {
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
