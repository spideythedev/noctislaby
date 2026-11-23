#pragma once

#include "object/object.h"
#include "object/signal.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nle {

class SceneTree;

class Node : public Object {
public:
    enum class TreeState {
        Outside,
        Entering,
        Inside,
        Exiting
    };

    enum class ProcessMode {
        Inherit,
        Pausable,
        WhenPaused,
        Always,
        Disabled
    };

    explicit Node(std::string name = "Node");
    ~Node() override;

    [[nodiscard]] const char* get_class_name() const noexcept override;

    [[nodiscard]] bool set_name(std::string name);
    [[nodiscard]] std::string_view get_name() const noexcept;
    [[nodiscard]] std::string get_path() const;

    [[nodiscard]] Node* get_parent() const noexcept;
    [[nodiscard]] std::size_t get_child_count() const noexcept;
    [[nodiscard]] Node* get_child(std::size_t index) const noexcept;

    [[nodiscard]] Node* add_child(std::unique_ptr<Node> child);
    [[nodiscard]] std::unique_ptr<Node> remove_child(Node* child);

    [[nodiscard]] bool is_inside_tree() const noexcept;
    [[nodiscard]] TreeState get_tree_state() const noexcept;

    void set_process_enabled(bool enabled) noexcept;
    [[nodiscard]] bool is_process_enabled() const noexcept;
    void set_process_priority(std::int32_t priority) noexcept;
    [[nodiscard]] std::int32_t get_process_priority() const noexcept;
    void set_process_mode(ProcessMode mode) noexcept;
    [[nodiscard]] ProcessMode get_process_mode() const noexcept;

    [[nodiscard]] Signal<>& tree_entered_signal() noexcept;
    [[nodiscard]] Signal<>& ready_signal() noexcept;
    [[nodiscard]] Signal<>& tree_exiting_signal() noexcept;
    [[nodiscard]] Signal<>& tree_exited_signal() noexcept;

protected:
    virtual void _enter_tree();
    virtual void _ready();
    virtual void _process(double delta);
    virtual void _physics_process(double delta);
    virtual void _exit_tree();

private:
    friend class SceneTree;

    void enter_tree_recursive();
    void ready_recursive();
    void exit_tree_recursive();
    void process_recursive(double delta);
    void physics_process_recursive(double delta);

    [[nodiscard]] std::vector<ObjectID> snapshot_children() const;
    [[nodiscard]] static Node* resolve_node(ObjectID id) noexcept;
    [[nodiscard]] bool has_child_name(std::string_view name) const noexcept;

    std::string name_;
    Node* parent_ = nullptr;
    std::vector<std::unique_ptr<Node>> children_;
    TreeState tree_state_ = TreeState::Outside;
    bool process_enabled_ = true;
    std::int32_t process_priority_ = 0;
    ProcessMode process_mode_ = ProcessMode::Inherit;

    Signal<> tree_entered_;
    Signal<> ready_;
    Signal<> tree_exiting_;
    Signal<> tree_exited_;
};

} // namespace nle
