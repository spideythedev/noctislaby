#pragma once

#include "node.h"

#include <memory>

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

    [[nodiscard]] bool is_initialized() const noexcept;

private:
    std::unique_ptr<Node> root_;
    bool initialized_ = false;
};

} // namespace nle
