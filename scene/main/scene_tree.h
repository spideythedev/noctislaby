#pragma once

#include "node.h"

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace nle {

class SceneTree final {
public:
    SceneTree();
    ~SceneTree();

    SceneTree(const SceneTree&) = delete;
    SceneTree& operator=(const SceneTree&) = delete;

    [[nodiscard]] bool set_root(std::unique_ptr<Node> root);
    [[nodiscard]] Node* get_root() const noexcept;

    [[nodiscard]] bool initialize();
    void process_frame(double delta);
    void physics_frame(double delta);
    void set_paused(bool paused) noexcept;
    [[nodiscard]] bool is_paused() const noexcept;
    void shutdown();

    [[nodiscard]] std::vector<Node*> get_nodes_in_group(std::string_view group) const;
    [[nodiscard]] std::size_t call_group(
        std::string_view group,
        const std::function<void(Node&)>& callback
    ) const;
    [[nodiscard]] bool call_deferred(
        Node* node,
        std::function<void(Node&)> callback
    );
    [[nodiscard]] std::size_t get_deferred_call_count() const noexcept;

    [[nodiscard]] bool queue_free(Node* node);
    [[nodiscard]] std::size_t get_queued_deletion_count() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    struct DeferredCall {
        ObjectID target{};
        std::function<void(Node&)> callback;
    };

    void flush_deferred_calls();
    void flush_deferred_deletions();
    [[nodiscard]] std::vector<ObjectID> get_processing_order() const;
    [[nodiscard]] bool can_process_node(const Node& node) const noexcept;

    std::unique_ptr<Node> root_;
    std::vector<ObjectID> deletion_queue_;
    std::vector<DeferredCall> deferred_calls_;
    bool initialized_ = false;
    bool paused_ = false;
};

} // namespace nle
