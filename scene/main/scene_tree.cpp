#include "scene_tree.h"

#include <utility>

namespace nle {

SceneTree::SceneTree()
    : root_(std::make_unique<Node>("Root")) {}

SceneTree::~SceneTree() {
    shutdown();
}

bool SceneTree::set_root(std::unique_ptr<Node> root) {
    if (initialized_ || root == nullptr || root->get_parent() != nullptr ||
        root->is_inside_tree()) {
        return false;
    }

    root_ = std::move(root);
    return true;
}

Node* SceneTree::get_root() const noexcept {
    return root_.get();
}

bool SceneTree::initialize() {
    if (initialized_ || root_ == nullptr) {
        return false;
    }

    initialized_ = true;
    root_->enter_tree_recursive();
    root_->ready_recursive();
    return true;
}

void SceneTree::process_frame(const double delta) {
    if (initialized_ && root_ != nullptr) {
        root_->process_recursive(delta);
    }
}

void SceneTree::physics_frame(const double delta) {
    if (initialized_ && root_ != nullptr) {
        root_->physics_process_recursive(delta);
    }
}

void SceneTree::shutdown() {
    if (!initialized_) {
        return;
    }

    if (root_ != nullptr) {
        root_->exit_tree_recursive();
    }

    initialized_ = false;
}

bool SceneTree::is_initialized() const noexcept {
    return initialized_;
}

} // namespace nle
