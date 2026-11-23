#include "scene_tree.h"

#include "object/object_db.h"

#include <algorithm>
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
    deletion_queue_.clear();
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
        flush_deferred_deletions();
    }
}

void SceneTree::physics_frame(const double delta) {
    if (initialized_ && root_ != nullptr) {
        root_->physics_process_recursive(delta);
        flush_deferred_deletions();
    }
}

void SceneTree::shutdown() {
    deletion_queue_.clear();
    if (!initialized_) {
        return;
    }

    if (root_ != nullptr) {
        root_->exit_tree_recursive();
    }

    initialized_ = false;
}

bool SceneTree::queue_free(Node* node) {
    if (!initialized_ || node == nullptr || node == root_.get() ||
        !node->is_inside_tree() || node->get_parent() == nullptr) {
        return false;
    }

    Node* ancestor = node;
    while (ancestor != nullptr && ancestor != root_.get()) {
        ancestor = ancestor->get_parent();
    }
    if (ancestor != root_.get()) {
        return false;
    }

    const ObjectID id = node->get_instance_id();
    const auto found = std::find(deletion_queue_.begin(), deletion_queue_.end(), id);
    if (found != deletion_queue_.end()) {
        return false;
    }

    deletion_queue_.push_back(id);
    return true;
}

std::size_t SceneTree::get_queued_deletion_count() const noexcept {
    return deletion_queue_.size();
}

bool SceneTree::is_initialized() const noexcept {
    return initialized_;
}

void SceneTree::flush_deferred_deletions() {
    while (!deletion_queue_.empty()) {
        std::vector<ObjectID> pending;
        pending.swap(deletion_queue_);

        for (const ObjectID id : pending) {
            auto* node = dynamic_cast<Node*>(ObjectDB::get_instance(id));
            if (node == nullptr || node == root_.get()) {
                continue;
            }

            Node* parent = node->get_parent();
            if (parent == nullptr) {
                continue;
            }

            static_cast<void>(parent->remove_child(node));
        }
    }
}

} // namespace nle
