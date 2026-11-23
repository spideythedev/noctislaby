#include "node.h"

#include "object/object_db.h"

#include <algorithm>
#include <utility>

namespace nle {

Node::Node(std::string name) : name_(std::move(name)) {
    if (name_.empty() || name_.find('/') != std::string::npos) {
        name_ = "Node";
    }
}

Node::~Node() {
    if (is_inside_tree()) {
        exit_tree_recursive();
    }
}

const char* Node::get_class_name() const noexcept {
    return "Node";
}

bool Node::set_name(std::string name) {
    if (name.empty() || name.find('/') != std::string::npos) {
        return false;
    }

    if (name_ == name) {
        return true;
    }

    if (parent_ != nullptr && parent_->has_child_name(name)) {
        return false;
    }

    name_ = std::move(name);
    return true;
}

std::string_view Node::get_name() const noexcept {
    return name_;
}

std::string Node::get_path() const {
    std::vector<std::string_view> parts;

    for (const Node* current = this; current != nullptr; current = current->parent_) {
        parts.push_back(current->name_);
    }

    std::string path;
    for (auto part = parts.rbegin(); part != parts.rend(); ++part) {
        path.push_back('/');
        path.append(*part);
    }

    return path;
}

Node* Node::get_parent() const noexcept {
    return parent_;
}

std::size_t Node::get_child_count() const noexcept {
    return children_.size();
}

Node* Node::get_child(const std::size_t index) const noexcept {
    return index < children_.size() ? children_[index].get() : nullptr;
}

Node* Node::add_child(std::unique_ptr<Node> child) {
    if (child == nullptr || child.get() == this || child->parent_ != nullptr ||
        child->is_inside_tree() || has_child_name(child->get_name())) {
        return nullptr;
    }

    for (Node* ancestor = this; ancestor != nullptr; ancestor = ancestor->parent_) {
        if (ancestor == child.get()) {
            return nullptr;
        }
    }

    Node* raw_child = child.get();
    children_.push_back(std::move(child));
    raw_child->parent_ = this;

    if (tree_state_ == TreeState::Entering || tree_state_ == TreeState::Inside) {
        raw_child->enter_tree_recursive();
        raw_child->ready_recursive();
    }

    return raw_child;
}

std::unique_ptr<Node> Node::remove_child(Node* child) {
    const auto found = std::find_if(
        children_.begin(),
        children_.end(),
        [child](const std::unique_ptr<Node>& candidate) {
            return candidate.get() == child;
        }
    );

    if (found == children_.end()) {
        return nullptr;
    }

    if ((*found)->is_inside_tree()) {
        (*found)->exit_tree_recursive();
    }

    std::unique_ptr<Node> removed = std::move(*found);
    children_.erase(found);
    removed->parent_ = nullptr;

    return removed;
}

bool Node::is_inside_tree() const noexcept {
    return tree_state_ != TreeState::Outside;
}

Node::TreeState Node::get_tree_state() const noexcept {
    return tree_state_;
}

void Node::set_process_enabled(const bool enabled) noexcept {
    process_enabled_ = enabled;
}

bool Node::is_process_enabled() const noexcept {
    return process_enabled_;
}

void Node::set_process_priority(const std::int32_t priority) noexcept {
    process_priority_ = priority;
}

std::int32_t Node::get_process_priority() const noexcept {
    return process_priority_;
}

void Node::set_process_mode(const ProcessMode mode) noexcept {
    process_mode_ = mode;
}

Node::ProcessMode Node::get_process_mode() const noexcept {
    return process_mode_;
}

bool Node::add_to_group(std::string group) {
    if (group.empty()) {
        return false;
    }

    if (is_in_group(group)) {
        return false;
    }

    groups_.push_back(std::move(group));
    return true;
}

bool Node::remove_from_group(const std::string_view group) {
    const auto found = std::find(groups_.begin(), groups_.end(), group);
    if (found == groups_.end()) {
        return false;
    }

    groups_.erase(found);
    return true;
}

bool Node::is_in_group(const std::string_view group) const noexcept {
    return std::find(groups_.begin(), groups_.end(), group) != groups_.end();
}

Signal<>& Node::tree_entered_signal() noexcept {
    return tree_entered_;
}

Signal<>& Node::ready_signal() noexcept {
    return ready_;
}

Signal<>& Node::tree_exiting_signal() noexcept {
    return tree_exiting_;
}

Signal<>& Node::tree_exited_signal() noexcept {
    return tree_exited_;
}

void Node::_enter_tree() {}
void Node::_ready() {}
void Node::_process(const double) {}
void Node::_physics_process(const double) {}
void Node::_exit_tree() {}

void Node::enter_tree_recursive() {
    if (tree_state_ != TreeState::Outside) {
        return;
    }

    tree_state_ = TreeState::Entering;
    _enter_tree();
    tree_state_ = TreeState::Inside;
    tree_entered_.emit();

    const std::vector<ObjectID> child_ids = snapshot_children();
    for (const ObjectID id : child_ids) {
        Node* child = resolve_node(id);
        if (child != nullptr && child->parent_ == this) {
            child->enter_tree_recursive();
        }
    }
}

void Node::ready_recursive() {
    if (tree_state_ != TreeState::Inside) {
        return;
    }

    const std::vector<ObjectID> child_ids = snapshot_children();
    for (const ObjectID id : child_ids) {
        Node* child = resolve_node(id);
        if (child != nullptr && child->parent_ == this) {
            child->ready_recursive();
        }
    }

    if (tree_state_ == TreeState::Inside) {
        _ready();
        ready_.emit();
    }
}

void Node::exit_tree_recursive() {
    if (tree_state_ == TreeState::Outside || tree_state_ == TreeState::Exiting) {
        return;
    }

    tree_state_ = TreeState::Exiting;
    tree_exiting_.emit();

    const std::vector<ObjectID> child_ids = snapshot_children();
    for (const ObjectID id : child_ids) {
        Node* child = resolve_node(id);
        if (child != nullptr && child->parent_ == this) {
            child->exit_tree_recursive();
        }
    }

    _exit_tree();
    tree_state_ = TreeState::Outside;
    tree_exited_.emit();
}

void Node::process_recursive(const double delta) {
    if (tree_state_ != TreeState::Inside) {
        return;
    }

    if (process_enabled_) {
        _process(delta);
    }

    const std::vector<ObjectID> child_ids = snapshot_children();
    for (const ObjectID id : child_ids) {
        Node* child = resolve_node(id);
        if (child != nullptr && child->parent_ == this) {
            child->process_recursive(delta);
        }
    }
}

void Node::physics_process_recursive(const double delta) {
    if (tree_state_ != TreeState::Inside) {
        return;
    }

    if (process_enabled_) {
        _physics_process(delta);
    }

    const std::vector<ObjectID> child_ids = snapshot_children();
    for (const ObjectID id : child_ids) {
        Node* child = resolve_node(id);
        if (child != nullptr && child->parent_ == this) {
            child->physics_process_recursive(delta);
        }
    }
}

std::vector<ObjectID> Node::snapshot_children() const {
    std::vector<ObjectID> ids;
    ids.reserve(children_.size());

    for (const auto& child : children_) {
        ids.push_back(child->get_instance_id());
    }

    return ids;
}

Node* Node::resolve_node(const ObjectID id) noexcept {
    return dynamic_cast<Node*>(ObjectDB::get_instance(id));
}

bool Node::has_child_name(const std::string_view name) const noexcept {
    return std::any_of(
        children_.begin(),
        children_.end(),
        [name](const std::unique_ptr<Node>& child) {
            return child->get_name() == name;
        }
    );
}

} // namespace nle
