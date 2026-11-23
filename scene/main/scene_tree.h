#pragma once

#include "node.h"

#include <memory>
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
    void shutdown();

    [[nodiscard]] bool queue_free(Node* node);
    [[nodiscard]] std::size_t get_queued_deletion_count() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    void flush_deferred_deletions();

    std::unique_ptr<Node> root_;
    std::vector<ObjectID> deletion_queue_;
    bool initialized_ = false;
};

} // namespace nle
