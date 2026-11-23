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
    pending_scene_.reset();
    deletion_queue_.clear();
    return true;
}

bool SceneTree::change_scene(std::unique_ptr<Node> scene) {
    if (!initialized_ || scene == nullptr || scene->get_parent() != nullptr ||
        scene->is_inside_tree() || pending_scene_ != nullptr) {
        return false;
    }

    pending_scene_ = std::move(scene);
    return true;
}

bool SceneTree::has_pending_scene_change() const noexcept {
    return pending_scene_ != nullptr;
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
        const std::vector<ObjectID> order = get_processing_order();
        for (const ObjectID id : order) {
            auto* node = dynamic_cast<Node*>(ObjectDB::get_instance(id));
            if (node != nullptr && node->is_inside_tree() && node->process_enabled_ &&
                can_process_node(*node)) {
                node->_process(delta);
            }
        }
        flush_deferred_calls();
        flush_deferred_deletions();
        apply_pending_scene_change();
    }
}

void SceneTree::physics_frame(const double delta) {
    if (initialized_ && root_ != nullptr) {
        const std::vector<ObjectID> order = get_processing_order();
        for (const ObjectID id : order) {
            auto* node = dynamic_cast<Node*>(ObjectDB::get_instance(id));
            if (node != nullptr && node->is_inside_tree() && node->process_enabled_ &&
                can_process_node(*node)) {
                node->_physics_process(delta);
            }
        }
        flush_deferred_calls();
        flush_deferred_deletions();
        apply_pending_scene_change();
    }
}

void SceneTree::set_paused(const bool paused) noexcept {
    paused_ = paused;
}

bool SceneTree::is_paused() const noexcept {
    return paused_;
}

void SceneTree::shutdown() {
    deletion_queue_.clear();
    deferred_calls_.clear();
    pending_scene_.reset();
    if (!initialized_) {
        return;
    }

    if (root_ != nullptr) {
        root_->exit_tree_recursive();
    }

    initialized_ = false;
}

std::vector<Node*> SceneTree::get_nodes_in_group(const std::string_view group) const {
    std::vector<Node*> matches;
    if (!initialized_ || root_ == nullptr || group.empty()) {
        return matches;
    }

    const std::vector<ObjectID> order = get_processing_order();
    matches.reserve(order.size());
    for (const ObjectID id : order) {
        Node* node = Node::resolve_node(id);
        if (node != nullptr && node->is_inside_tree() && node->is_in_group(group)) {
            matches.push_back(node);
        }
    }
    return matches;
}

std::size_t SceneTree::call_group(
    const std::string_view group,
    const std::function<void(Node&)>& callback
) const {
    if (!callback || group.empty() || !initialized_ || root_ == nullptr) {
        return 0;
    }

    std::vector<ObjectID> snapshot;
    for (Node* node : get_nodes_in_group(group)) {
        snapshot.push_back(node->get_instance_id());
    }

    std::size_t called = 0;
    for (const ObjectID id : snapshot) {
        Node* node = Node::resolve_node(id);
        if (node == nullptr || !node->is_inside_tree() || !node->is_in_group(group)) {
            continue;
        }

        callback(*node);
        ++called;
    }
    return called;
}

bool SceneTree::call_deferred(
    Node* node,
    std::function<void(Node&)> callback
) {
    if (!initialized_ || node == nullptr || !callback || !node->is_inside_tree()) {
        return false;
    }

    Node* ancestor = node;
    while (ancestor != nullptr && ancestor != root_.get()) {
        ancestor = ancestor->get_parent();
    }
    if (ancestor != root_.get()) {
        return false;
    }

    deferred_calls_.push_back({node->get_instance_id(), std::move(callback)});
    return true;
}

std::size_t SceneTree::get_deferred_call_count() const noexcept {
    return deferred_calls_.size();
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

bool SceneTree::can_process_node(const Node& node) const noexcept {
    for (const Node* current = &node; current != nullptr; current = current->parent_) {
        switch (current->process_mode_) {
            case Node::ProcessMode::Inherit:
                continue;
            case Node::ProcessMode::Pausable:
                return !paused_;
            case Node::ProcessMode::WhenPaused:
                return paused_;
            case Node::ProcessMode::Always:
                return true;
            case Node::ProcessMode::Disabled:
            default:
                return false;
        }
    }

    return !paused_;
}

std::vector<ObjectID> SceneTree::get_processing_order() const {
    std::vector<ObjectID> order;
    if (root_ == nullptr) {
        return order;
    }

    std::vector<ObjectID> pending{root_->get_instance_id()};
    while (!pending.empty()) {
        const ObjectID id = pending.back();
        pending.pop_back();

        Node* node = Node::resolve_node(id);
        if (node == nullptr || !node->is_inside_tree()) {
            continue;
        }

        order.push_back(id);
        const std::vector<ObjectID> children = node->snapshot_children();
        for (auto child = children.rbegin(); child != children.rend(); ++child) {
            pending.push_back(*child);
        }
    }

    std::stable_sort(order.begin(), order.end(), [](const ObjectID left, const ObjectID right) {
        const Node* left_node = Node::resolve_node(left);
        const Node* right_node = Node::resolve_node(right);
        if (left_node == nullptr || right_node == nullptr) {
            return false;
        }
        return left_node->get_process_priority() < right_node->get_process_priority();
    });
    return order;
}

void SceneTree::flush_deferred_calls() {
    std::vector<DeferredCall> pending;
    pending.swap(deferred_calls_);

    for (DeferredCall& call : pending) {
        Node* node = Node::resolve_node(call.target);
        if (node == nullptr || !node->is_inside_tree()) {
            continue;
        }

        call.callback(*node);
    }
}

void SceneTree::apply_pending_scene_change() {
    if (!initialized_ || pending_scene_ == nullptr) {
        return;
    }

    std::unique_ptr<Node> next_scene = std::move(pending_scene_);
    if (root_ != nullptr) {
        root_->exit_tree_recursive();
    }

    root_ = std::move(next_scene);
    deletion_queue_.clear();
    deferred_calls_.clear();
    root_->enter_tree_recursive();
    root_->ready_recursive();
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
