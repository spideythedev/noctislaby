#include "error/error.h"
#include "config/project_config.h"
#include "config/project.h"
#include "io/file_access.h"
#include "io/resource.h"
#include "io/resource_cache.h"
#include "io/resource_loader.h"
#include "io/resource_saver.h"
#include "io/resource_uid_registry.h"
#include "io/resource_path.h"
#include "object/class_db.h"
#include "math/aabb.h"
#include "math/basis.h"
#include "math/color.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include "math/rect2.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "math/transform2d.h"
#include "math/transform3d.h"
#include "memory/memory.h"
#include "object/object.h"
#include "object/object_db.h"
#include "object/ref_counted.h"
#include "object/signal.h"
#include "servers/rendering/null/null_rendering_device.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/render_graph.h"
#include "servers/input/input_server.h"
#include "servers/input/input_map.h"
#include "servers/physics/physics_server_2d.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/node_3d.h"
#include "scene/main/node.h"
#include "scene/main/main_loop.h"
#include "scene/main/scene_tree.h"
#include "string/string_name.h"
#include "variant/variant.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <cmath>
#include <limits>
#include <filesystem>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <unordered_map>
#include <utility>
#include <chrono>

namespace {

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class TestObject final : public nle::Object {
public:
    [[nodiscard]] const char* get_class_name() const noexcept override {
        return "TestObject";
    }
};

class TestTexture final : public nle::Resource {};

class LifecycleNode final : public nle::Node {
public:
    LifecycleNode(std::string name, std::vector<std::string>& events)
        : Node(std::move(name)), events_(events) {}

protected:
    void _enter_tree() override {
        events_.push_back(std::string(get_name()) + ".enter");
    }

    void _ready() override {
        events_.push_back(std::string(get_name()) + ".ready");
    }

    void _process(const double) override {
        events_.push_back(std::string(get_name()) + ".process");
    }

    void _physics_process(const double) override {
        events_.push_back(std::string(get_name()) + ".physics");
    }

    void _exit_tree() override {
        events_.push_back(std::string(get_name()) + ".exit");
    }

private:
    std::vector<std::string>& events_;
};

class TestFormatSaver final : public nle::ResourceFormatSaver {
public:
    [[nodiscard]] std::vector<std::string> get_recognized_extensions() const override {
        return {"nleres"};
    }

    [[nodiscard]] nle::ErrorCode save(
        const std::string_view path,
        const nle::Ref<nle::Resource>& resource
    ) override {
        if (!resource) {
            return nle::ErrorCode::InvalidParameter;
        }

        ++save_count;
        saved_path = std::string(path);
        return nle::ErrorCode::Ok;
    }

    int save_count = 0;
    std::string saved_path;
};

class TestFormatLoader final : public nle::ResourceFormatLoader {
public:
    [[nodiscard]] std::vector<std::string> get_recognized_extensions() const override {
        return {"nleres"};
    }

    [[nodiscard]] nle::Ref<nle::Resource> load(
        const std::string_view,
        nle::ErrorCode& error
    ) override {
        ++load_count;
        error = nle::ErrorCode::Ok;
        return nle::make_ref<nle::Resource>();
    }

    int load_count = 0;
};

class ConcurrentTestFormatLoader final : public nle::ResourceFormatLoader {
public:
    [[nodiscard]] std::vector<std::string> get_recognized_extensions() const override {
        return {"nleres"};
    }

    [[nodiscard]] nle::Ref<nle::Resource> load(
        const std::string_view,
        nle::ErrorCode& error
    ) override {
        ++load_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        error = nle::ErrorCode::Ok;
        return nle::make_ref<nle::Resource>();
    }

    std::atomic<int> load_count{0};
};

class TestResource final : public nle::RefCounted {
public:
    explicit TestResource(int* destruction_count)
        : destruction_count_(destruction_count) {}

    ~TestResource() override {
        ++(*destruction_count_);
    }

private:
    int* destruction_count_;
};

void test_input_map() {
    nle::InputServer input;
    nle::InputMap input_map;
    const nle::InputControl key_space{nle::InputEventType::Key, 0, 32};
    const nle::InputControl mouse_left{nle::InputEventType::MouseButton, 0, 1};

    expect(input_map.add_action("jump"), "input map registers an action");
    expect(!input_map.add_action("jump"), "input map rejects duplicate actions");
    expect(!input_map.add_binding("missing", key_space),
           "input map rejects bindings for unknown actions");
    expect(input_map.add_binding("jump", key_space),
           "input map binds a keyboard key");
    expect(input_map.add_binding("jump", mouse_left),
           "input map supports multiple bindings per action");
    expect(!input_map.add_binding("jump", key_space),
           "input map rejects duplicate bindings");
    expect(input_map.get_bindings("jump").size() == 2,
           "input map returns action bindings");

    nle::InputEvent press;
    press.type = nle::InputEventType::Key;
    press.code = 32;
    press.pressed = true;
    expect(input.push_event(press), "input map test queues key press");
    input.process_frame();

    expect(input_map.get_action_strength(input, "jump") == nle::real_t{1},
           "input map reports digital action strength");
    expect(input_map.is_action_pressed(input, "jump"),
           "input map reports pressed action state");
    expect(input_map.is_action_just_pressed(input, "jump"),
           "input map reports action press edge");

    input.process_frame();
    expect(!input_map.is_action_just_pressed(input, "jump"),
           "input map clears action press edge on next frame");

    press.pressed = false;
    expect(input.push_event(press), "input map test queues key release");
    input.process_frame();
    expect(!input_map.is_action_pressed(input, "jump") &&
           input_map.is_action_just_released(input, "jump"),
           "input map reports action release edge");

    expect(input_map.remove_binding("jump", mouse_left),
           "input map removes an action binding");
    expect(!input_map.remove_binding("jump", mouse_left),
           "input map rejects duplicate binding removal");
    expect(input_map.remove_action("jump"),
           "input map removes an action");
    expect(!input_map.has_action("jump"),
           "input map no longer reports removed action");
}

void test_physics_server_2d() {
    nle::PhysicsServer2D physics;
    nle::PhysicsBodyDescription2D floor_description;
    floor_description.mode = nle::PhysicsBodyMode2D::Static;
    floor_description.shape = {{0, 0}, {20, 1}};
    nle::RID floor;
    expect(physics.create_body(floor_description, floor) == nle::ErrorCode::Ok,
           "2D physics creates a static floor");

    nle::PhysicsBodyDescription2D box_description;
    box_description.mode = nle::PhysicsBodyMode2D::Dynamic;
    box_description.shape = {{0, 0}, {1, 1}};
    box_description.position = {2, 0.5F};
    box_description.velocity = {0, -1};
    box_description.mass = 1;
    nle::RID box;
    expect(physics.create_body(box_description, box) == nle::ErrorCode::Ok,
           "2D physics creates a dynamic body");
    expect(physics.get_body_count() == 2, "2D physics tracks body count");

    expect(physics.step(1.0 / 60.0, {0, -9.8F}) == nle::ErrorCode::Ok,
           "2D physics advances a simulation step");
    nle::PhysicsBodyState2D state;
    expect(physics.get_body_state(box, state) == nle::ErrorCode::Ok,
           "2D physics returns body state");
    expect(state.world_bounds.position.y >= 0.99F,
           "2D physics resolves dynamic body penetration against static floor");
    expect(state.velocity.y >= -0.001F,
           "2D physics removes velocity into the collision normal");

    nle::PhysicsBodyDescription2D falling_description;
    falling_description.mode = nle::PhysicsBodyMode2D::Dynamic;
    falling_description.shape = {{0, 0}, {1, 1}};
    falling_description.position = {30, 10};
    falling_description.mass = 2;
    nle::RID falling;
    expect(physics.create_body(falling_description, falling) == nle::ErrorCode::Ok,
           "2D physics creates a separate falling body");
    expect(physics.step(0.1, {0, -10}) == nle::ErrorCode::Ok,
           "2D physics integrates gravity");
    expect(physics.get_body_state(falling, state) == nle::ErrorCode::Ok &&
           state.velocity.y < 0 && state.position.y < 10,
           "2D physics updates velocity and position under gravity");

    expect(physics.step(0.5, {0, -9.8F}) == nle::ErrorCode::InvalidParameter,
           "2D physics rejects excessive variable steps");
    expect(physics.set_body_velocity(falling, {
        std::numeric_limits<nle::real_t>::infinity(), 0
    }) == nle::ErrorCode::InvalidParameter,
           "2D physics rejects non-finite body velocities");
    expect(physics.free_body(box) == nle::ErrorCode::Ok,
           "2D physics frees a body");
    expect(physics.free_body(box) == nle::ErrorCode::NotFound,
           "2D physics rejects stale body identifiers");
    expect(physics.get_body_count() == 2,
           "2D physics updates body count after removal");
}

void test_input_server() {
    nle::InputServer input;
    const nle::InputControl key_a{nle::InputEventType::Key, 0, 65};
    int event_count = 0;
    const nle::ConnectionID connection = input.event_signal().connect(
        [&event_count](const nle::InputEvent&) { ++event_count; }
    );

    nle::InputEvent key_press;
    key_press.type = nle::InputEventType::Key;
    key_press.code = 65;
    key_press.pressed = true;
    expect(input.push_event(key_press), "input server queues key press");

    nle::InputEvent mouse_motion;
    mouse_motion.type = nle::InputEventType::MouseMotion;
    mouse_motion.position = {120, 240};
    mouse_motion.relative = {5, -3};
    expect(input.push_event(mouse_motion), "input server queues mouse motion");
    expect(input.get_queued_event_count() == 2,
           "input server reports queued event count");

    input.process_frame();
    expect(input.is_pressed(key_a) && input.is_just_pressed(key_a),
           "input server updates pressed and just-pressed state");
    expect(input.get_mouse_position() == nle::Vector2{120, 240},
           "input server updates mouse position");
    expect(event_count == 2 && input.get_queued_event_count() == 0,
           "input server emits drained events outside its queue");

    input.process_frame();
    expect(input.is_pressed(key_a) && !input.is_just_pressed(key_a),
           "input server clears edge state at the next frame");

    nle::InputEvent key_release = key_press;
    key_release.pressed = false;
    expect(input.push_event(key_release), "input server queues key release");
    input.process_frame();
    expect(!input.is_pressed(key_a) && input.is_just_released(key_a),
           "input server updates just-released state");

    nle::InputEvent invalid_axis;
    invalid_axis.type = nle::InputEventType::JoypadAxis;
    invalid_axis.device = 1;
    invalid_axis.code = 0;
    invalid_axis.value = std::numeric_limits<nle::real_t>::infinity();
    expect(!input.push_event(invalid_axis),
           "input server rejects non-finite controller axis values");

    nle::InputEvent invalid_type;
    invalid_type.type = static_cast<nle::InputEventType>(255);
    expect(!input.push_event(invalid_type),
           "input server rejects unknown event types");

    expect(input.event_signal().disconnect(connection),
           "input event listener can be disconnected");
}

void test_render_graph() {
    nle::RenderGraph graph;
    nle::RenderGraphResourceId source = nle::RenderGraph::InvalidResource;
    nle::RenderGraphResourceId intermediate = nle::RenderGraph::InvalidResource;
    nle::RenderGraphResourceId output = nle::RenderGraph::InvalidResource;

    expect(graph.create_resource("swapchain", true, source) == nle::ErrorCode::Ok,
           "render graph imports external resources");
    expect(graph.create_resource("scene-color", false, intermediate) == nle::ErrorCode::Ok,
           "render graph creates transient resources");
    expect(graph.create_resource("composite", false, output) == nle::ErrorCode::Ok,
           "render graph creates output resources");

    const std::array<nle::RenderGraphResourceUse, 1> geometry_uses{{
        {source, nle::RenderGraphAccess::Read}
    }};
    const std::array<nle::RenderGraphResourceUse, 2> lighting_uses{{
        {source, nle::RenderGraphAccess::Read},
        {intermediate, nle::RenderGraphAccess::Write}
    }};
    const std::array<nle::RenderGraphResourceUse, 2> composite_uses{{
        {intermediate, nle::RenderGraphAccess::Read},
        {output, nle::RenderGraphAccess::Write}
    }};

    nle::RenderGraphPassId geometry = nle::RenderGraph::InvalidPass;
    nle::RenderGraphPassId lighting = nle::RenderGraph::InvalidPass;
    nle::RenderGraphPassId composite = nle::RenderGraph::InvalidPass;
    expect(graph.add_pass("geometry", geometry_uses, geometry) == nle::ErrorCode::Ok,
           "render graph adds geometry pass");
    expect(graph.add_pass("lighting", lighting_uses, lighting) == nle::ErrorCode::Ok,
           "render graph adds lighting pass");
    expect(graph.add_pass("composite", composite_uses, composite) == nle::ErrorCode::Ok,
           "render graph adds composite pass");

    std::vector<nle::RenderGraphPassId> order;
    expect(graph.compile(order) == nle::ErrorCode::Ok,
           "render graph compiles resource dependencies");
    expect(order == std::vector<nle::RenderGraphPassId>{geometry, lighting, composite},
           "render graph produces deterministic dependency order");

    nle::RenderGraph invalid_read;
    nle::RenderGraphResourceId transient = 0;
    expect(invalid_read.create_resource("uninitialized", false, transient) ==
           nle::ErrorCode::Ok, "render graph creates uninitialized transient");
    const std::array<nle::RenderGraphResourceUse, 1> read_before_write{{
        {transient, nle::RenderGraphAccess::Read}
    }};
    nle::RenderGraphPassId read_pass = 0;
    expect(invalid_read.add_pass("invalid-read", read_before_write, read_pass) ==
           nle::ErrorCode::Ok, "render graph accepts pass declaration");
    expect(invalid_read.compile(order) == nle::ErrorCode::InvalidState && order.empty(),
           "render graph rejects transient read before first write");

    nle::RenderGraph cyclic;
    nle::RenderGraphPassId first = 0;
    nle::RenderGraphPassId second = 0;
    expect(cyclic.add_pass("first", {}, first) == nle::ErrorCode::Ok,
           "render graph adds first independent pass");
    expect(cyclic.add_pass("second", {}, second) == nle::ErrorCode::Ok,
           "render graph adds second independent pass");
    expect(cyclic.add_dependency(first, second) == nle::ErrorCode::Ok,
           "render graph adds explicit dependency");
    expect(cyclic.add_dependency(second, first) == nle::ErrorCode::Ok,
           "render graph accepts dependency for cycle validation");
    expect(cyclic.compile(order) == nle::ErrorCode::InvalidState && order.empty(),
           "render graph detects dependency cycles");

    nle::RenderGraph reordered;
    nle::RenderGraphPassId earlier_declared = 0;
    nle::RenderGraphPassId prerequisite = 0;
    expect(reordered.add_pass("consumer", {}, earlier_declared) == nle::ErrorCode::Ok,
           "render graph declares consumer before prerequisite");
    expect(reordered.add_pass("producer", {}, prerequisite) == nle::ErrorCode::Ok,
           "render graph declares prerequisite after consumer");
    expect(reordered.add_dependency(earlier_declared, prerequisite) == nle::ErrorCode::Ok,
           "render graph records forward dependency");
    expect(reordered.compile(order) == nle::ErrorCode::Ok &&
           order == std::vector<nle::RenderGraphPassId>{prerequisite, earlier_declared},
           "render graph honors forward dependencies deterministically");

    nle::RenderGraph invalid_declaration;
    nle::RenderGraphResourceId imported = 0;
    expect(invalid_declaration.create_resource("imported", true, imported) ==
           nle::ErrorCode::Ok, "render graph creates resource for access validation");
    const std::array<nle::RenderGraphResourceUse, 1> invalid_access{{
        {imported, static_cast<nle::RenderGraphAccess>(255)}
    }};
    nle::RenderGraphPassId invalid_pass = 0;
    expect(invalid_declaration.add_pass("invalid-access", invalid_access, invalid_pass) ==
           nle::ErrorCode::InvalidParameter,
           "render graph rejects unknown resource access values");

    expect(cyclic.add_dependency(first, first) == nle::ErrorCode::InvalidParameter,
           "render graph rejects self dependency");
    expect(graph.get_resource_count() == 3 && graph.get_pass_count() == 3,
           "render graph reports declared resource and pass counts");
    graph.clear();
    expect(graph.get_resource_count() == 0 && graph.get_pass_count() == 0,
           "render graph clears declarations");
}

void test_rendering_device() {
    nle::NullRenderingDevice device;
    nle::RID uninitialized_buffer;
    expect(device.create_buffer({}, uninitialized_buffer) == nle::ErrorCode::InvalidParameter,
           "null device validates buffer descriptions before initialization");
    expect(device.create_buffer({4, nle::BufferUsage::Vertex, {}}, uninitialized_buffer) ==
           nle::ErrorCode::InvalidState,
           "uninitialized rendering device rejects buffer creation");
    expect(device.initialize() == nle::ErrorCode::Ok,
           "null rendering device initializes");
    expect(device.initialize() == nle::ErrorCode::AlreadyExists,
           "null rendering device rejects duplicate initialization");

    const std::array<std::byte, 4> texture_initial{
        std::byte{10}, std::byte{20}, std::byte{30}, std::byte{40}
    };
    const nle::TextureDescription texture_description{
        2,
        1,
        nle::TextureFormat::RGBA8Unorm,
        nle::TextureUsage::Sampled | nle::TextureUsage::TransferDestination,
        texture_initial
    };
    nle::RID texture;
    expect(device.create_texture(texture_description, texture) == nle::ErrorCode::Ok,
           "null device creates a texture");
    expect(device.get_texture_count() == 1, "null device tracks allocated textures");

    std::array<std::byte, 8> texture_contents{};
    expect(device.read_texture(texture, 0, texture_contents) == nle::ErrorCode::Ok,
           "null device reads texture bytes");
    expect(std::equal(texture_initial.begin(), texture_initial.end(), texture_contents.begin()),
           "texture creation copies initial data");
    expect(texture_contents[4] == std::byte{0},
           "texture allocation zero-fills unspecified bytes");

    const std::array<std::byte, 2> texture_update{std::byte{90}, std::byte{80}};
    expect(device.update_texture(texture, 6, texture_update) == nle::ErrorCode::Ok,
           "null device updates texture byte range");
    expect(device.read_texture(texture, 6, std::span<std::byte>(texture_contents).subspan(6, 2)) ==
           nle::ErrorCode::Ok && texture_contents[6] == std::byte{90} &&
           texture_contents[7] == std::byte{80},
           "texture updates are visible to readback");
    expect(device.update_texture(texture, 7, texture_update) ==
           nle::ErrorCode::InvalidParameter,
           "texture update rejects out-of-range writes");
    nle::RID invalid_texture;
    expect(device.create_texture({0, 1, nle::TextureFormat::RGBA8Unorm, {}, {}},
                                 invalid_texture) == nle::ErrorCode::InvalidParameter,
           "null device rejects zero-width textures");
    const nle::RID stale_texture = texture;
    expect(device.free_texture(texture) == nle::ErrorCode::Ok,
           "null device frees textures");
    expect(device.free_texture(texture) == nle::ErrorCode::NotFound,
           "null device rejects stale texture identifiers");
    nle::RID reused_texture;
    expect(device.create_texture(texture_description, reused_texture) == nle::ErrorCode::Ok,
           "null device reuses released texture slots");
    expect(reused_texture.get_index() == stale_texture.get_index() &&
           reused_texture.get_generation() != stale_texture.get_generation(),
           "texture allocator advances generation when reusing a slot");
    expect(device.read_texture(stale_texture, 0, texture_contents) == nle::ErrorCode::NotFound,
           "stale texture RID cannot access a reused resource");
    expect(device.free_texture(reused_texture) == nle::ErrorCode::Ok,
           "null device frees reused texture");
    expect(device.get_texture_count() == 0, "freeing texture updates resource count");


    const std::array<std::byte, 4> initial{
        std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
    };
    const nle::BufferDescription description{
        8,
        nle::BufferUsage::Vertex | nle::BufferUsage::Readback,
        initial
    };

    nle::RID first;
    expect(device.create_buffer(description, first) == nle::ErrorCode::Ok,
           "null device creates a buffer");
    expect(first.is_valid(), "created buffer receives a valid RID");
    expect(device.get_buffer_count() == 1, "null device tracks allocated buffers");

    std::array<std::byte, 8> contents{};
    expect(device.read_buffer(first, 0, contents) == nle::ErrorCode::Ok,
           "null device reads buffer contents");
    expect(std::equal(initial.begin(), initial.end(), contents.begin()),
           "buffer creation copies initial data");
    expect(contents[4] == std::byte{0} && contents[7] == std::byte{0},
           "uninitialized buffer bytes are zero-filled");

    const std::array<std::byte, 2> update{std::byte{9}, std::byte{8}};
    expect(device.update_buffer(first, 2, update) == nle::ErrorCode::Ok,
           "null device updates buffer range");
    expect(device.read_buffer(first, 2, std::span<std::byte>(contents).subspan(2, 2)) ==
           nle::ErrorCode::Ok, "null device reads updated range");
    expect(contents[2] == std::byte{9} && contents[3] == std::byte{8},
           "buffer update changes only requested bytes");

    expect(device.update_buffer(first, 7, update) == nle::ErrorCode::InvalidParameter,
           "buffer update rejects out-of-range writes");
    nle::RID invalid_buffer;
    expect(device.create_buffer({0, nle::BufferUsage::Vertex, {}}, invalid_buffer) ==
           nle::ErrorCode::InvalidParameter,
           "null device rejects zero-sized buffers");

    const nle::RID stale = first;
    expect(device.free_buffer(first) == nle::ErrorCode::Ok,
           "null device frees buffer");
    expect(device.free_buffer(first) == nle::ErrorCode::NotFound,
           "null device rejects stale resource identifiers");
    expect(device.get_buffer_count() == 0, "freeing buffer updates resource count");

    nle::RID reused;
    expect(device.create_buffer(description, reused) == nle::ErrorCode::Ok,
           "null device reuses released buffer slot");
    expect(reused.get_index() == stale.get_index(),
           "resource allocator reuses released slot");
    expect(reused.get_generation() != stale.get_generation(),
           "resource allocator increments slot generation");
    expect(device.read_buffer(stale, 0, contents) == nle::ErrorCode::NotFound,
           "stale RID cannot access a reused resource");

    device.shutdown();
    expect(!device.is_initialized(), "null rendering device shuts down");
    expect(device.read_buffer(reused, 0, contents) == nle::ErrorCode::InvalidState,
           "shutdown device rejects resource access");


    nle::RenderingServer server;
    expect(server.create_buffer(description, reused) == nle::ErrorCode::InvalidState,
           "rendering server rejects operations before initialization");
    expect(server.initialize(std::make_unique<nle::NullRenderingDevice>()) ==
           nle::ErrorCode::Ok, "rendering server initializes selected backend");
    expect(server.is_initialized(), "rendering server reports initialized");
    const nle::RenderingDeviceInfo device_info = server.get_device_info();
    expect(device_info.name == "NoctisLaby Null Device",
           "rendering server exposes backend information");
    expect(server.initialize(std::make_unique<nle::NullRenderingDevice>()) ==
           nle::ErrorCode::AlreadyExists,
           "rendering server rejects a second backend");
    expect(server.create_buffer(description, reused) == nle::ErrorCode::Ok,
           "rendering server forwards buffer creation");
    nle::RID server_texture;
    expect(server.create_texture(texture_description, server_texture) == nle::ErrorCode::Ok,
           "rendering server forwards texture creation");
    expect(server.free_texture(server_texture) == nle::ErrorCode::Ok,
           "rendering server forwards texture release");

    expect(server.free_buffer(reused) == nle::ErrorCode::Ok,
           "rendering server forwards buffer release");
    server.shutdown();
    expect(!server.is_initialized(), "rendering server shuts down backend");
    expect(device_info.name == "NoctisLaby Null Device",
           "device metadata remains valid after backend shutdown");
}

void test_spatial_nodes() {
    nle::Node2D node_2d;
    node_2d.set_position({12, 24});
    expect(node_2d.get_position() == nle::Vector2{12, 24},
           "Node2D stores local position");
    expect(std::string_view(node_2d.get_class_name()) == "Node2D",
           "Node2D reports class name");

    nle::Node3D node_3d;
    node_3d.set_position({3, 6, 9});
    expect(node_3d.get_position() == nle::Vector3{3, 6, 9},
           "Node3D stores local position");
    expect(std::string_view(node_3d.get_class_name()) == "Node3D",
           "Node3D reports class name");

    const nle::Transform3D transform{
        nle::Basis{},
        {10, 20, 30}
    };
    node_3d.set_transform(transform);
    expect(node_3d.get_transform() == transform, "Node3D stores full transform");

    auto parent_2d = std::make_unique<nle::Node2D>("Parent2D");
    parent_2d->set_position({10, 20});
    auto child_2d = std::make_unique<nle::Node2D>("Child2D");
    child_2d->set_position({1, 2});
    nle::Node2D* child_2d_pointer = child_2d.get();
    expect(parent_2d->add_child(std::move(child_2d)) == child_2d_pointer,
           "Node2D accepts spatial child");
    expect(child_2d_pointer->get_global_transform().xform({0, 0}) ==
           nle::Vector2{11, 22}, "Node2D composes global transform");

    auto parent_3d = std::make_unique<nle::Node3D>("Parent3D");
    parent_3d->set_position({10, 20, 30});
    auto child_3d = std::make_unique<nle::Node3D>("Child3D");
    child_3d->set_position({1, 2, 3});
    nle::Node3D* child_3d_pointer = child_3d.get();
    expect(parent_3d->add_child(std::move(child_3d)) == child_3d_pointer,
           "Node3D accepts spatial child");
    expect(child_3d_pointer->get_global_transform().xform({0, 0, 0}) ==
           nle::Vector3{11, 22, 33}, "Node3D composes global transform");

    auto nested_root = std::make_unique<nle::Node2D>("NestedRoot");
    nested_root->set_position({5, 7});
    auto spacer = std::make_unique<nle::Node>("Spacer");
    auto leaf = std::make_unique<nle::Node2D>("Leaf");
    leaf->set_position({2, 3});
    nle::Node2D* leaf_pointer = leaf.get();
    nle::Node* spacer_pointer = nested_root->add_child(std::move(spacer));
    expect(spacer_pointer->add_child(std::move(leaf)) == leaf_pointer,
           "Node2D supports intermediate non-spatial nodes");
    expect(leaf_pointer->get_global_position() == nle::Vector2{7, 10},
           "Node2D inherits transform through non-spatial ancestor");
    expect(leaf_pointer->set_global_position({20, 30}),
           "Node2D sets global position");
    expect(leaf_pointer->get_global_position() == nle::Vector2{20, 30},
           "Node2D global position setter preserves requested position");

    auto singular_parent_2d = std::make_unique<nle::Node2D>("SingularParent2D");
    singular_parent_2d->set_transform({{0, 0}, {0, 1}, {0, 0}});
    auto singular_child_2d = std::make_unique<nle::Node2D>("SingularChild2D");
    nle::Node2D* singular_child_2d_pointer = singular_child_2d.get();
    expect(singular_parent_2d->add_child(std::move(singular_child_2d)) ==
           singular_child_2d_pointer, "singular 2D parent accepts child");
    expect(!singular_child_2d_pointer->set_global_position({1, 1}),
           "Node2D rejects global position under singular parent transform");

    auto nested_root_3d = std::make_unique<nle::Node3D>("NestedRoot3D");
    nested_root_3d->set_position({5, 7, 9});
    auto spacer_3d = std::make_unique<nle::Node>("Spacer3D");
    auto leaf_3d = std::make_unique<nle::Node3D>("Leaf3D");
    leaf_3d->set_position({2, 3, 4});
    nle::Node3D* leaf_3d_pointer = leaf_3d.get();
    nle::Node* spacer_3d_pointer = nested_root_3d->add_child(std::move(spacer_3d));
    expect(spacer_3d_pointer->add_child(std::move(leaf_3d)) == leaf_3d_pointer,
           "Node3D supports intermediate non-spatial nodes");
    expect(leaf_3d_pointer->get_global_position() == nle::Vector3{7, 10, 13},
           "Node3D inherits transform through non-spatial ancestor");
    expect(leaf_3d_pointer->set_global_position({20, 30, 40}),
           "Node3D sets global position");
    expect(leaf_3d_pointer->get_global_position() == nle::Vector3{20, 30, 40},
           "Node3D global position setter preserves requested position");

    auto singular_parent_3d = std::make_unique<nle::Node3D>("SingularParent3D");
    singular_parent_3d->set_transform({
        nle::Basis{{0, 0, 0}, {0, 1, 0}, {0, 0, 1}},
        {0, 0, 0}
    });
    auto singular_child_3d = std::make_unique<nle::Node3D>("SingularChild3D");
    nle::Node3D* singular_child_3d_pointer = singular_child_3d.get();
    expect(singular_parent_3d->add_child(std::move(singular_child_3d)) ==
           singular_child_3d_pointer, "singular 3D parent accepts child");
    expect(!singular_child_3d_pointer->set_global_position({1, 1, 1}),
           "Node3D rejects global position under singular parent transform");
}

void test_main_loop() {
    std::vector<std::string> events;
    auto root = std::make_unique<LifecycleNode>("Root", events);
    nle::SceneTree tree;
    expect(tree.set_root(std::move(root)), "main loop scene tree accepts root");
    expect(tree.initialize(), "main loop scene tree initializes");

    nle::MainLoop loop(tree);
    nle::MainLoopFrameStats stats;
    expect(loop.iteration(2.0 / 60.0, &stats), "main loop executes a valid frame");
    expect(stats.frame_index == 1 && stats.physics_steps == 2,
           "main loop advances fixed physics ticks");
    expect(stats.dropped_physics_steps == 0,
           "main loop does not drop physics steps within its budget");
    expect(stats.frame_delta > 0.0 && stats.interpolation_alpha < 0.000001,
           "main loop reports frame delta and interpolation state");
    expect(events == std::vector<std::string>{
        "Root.enter", "Root.ready", "Root.physics", "Root.physics", "Root.process"
    }, "main loop runs fixed physics before frame processing");

    events.clear();
    expect(loop.iteration(0.5, &stats), "main loop accepts long frames");
    expect(stats.frame_delta == 0.25 && stats.physics_steps == 8 &&
           stats.dropped_physics_steps == 7,
           "main loop clamps frame delta and bounds physics catch-up");
    expect(events.size() == 9,
           "main loop performs bounded physics and one process callback");

    events.clear();
    expect(loop.set_time_scale(0.0), "main loop supports paused time scale");
    expect(loop.iteration(0.1, &stats), "main loop continues rendering while paused");
    expect(stats.physics_steps == 0 && stats.frame_delta == 0.0,
           "paused time scale stops simulation time");
    expect(events == std::vector<std::string>{"Root.process"},
           "paused main loop still processes a frame");

    expect(!loop.set_physics_ticks_per_second(0),
           "main loop rejects zero physics frequency");
    expect(!loop.set_max_physics_steps_per_frame(0),
           "main loop rejects zero catch-up budget");
    expect(!loop.set_time_scale(-1.0),
           "main loop rejects negative time scale");
    expect(!loop.iteration(-0.1, &stats),
           "main loop rejects negative frame delta");

    loop.reset();
    expect(loop.get_frame_index() == 0 && loop.get_interpolation_alpha() == 0.0,
           "main loop reset clears frame and interpolation state");
    tree.shutdown();
}

void test_scene_tree() {
    std::vector<std::string> events;
    auto root = std::make_unique<LifecycleNode>("Root", events);
    auto child = std::make_unique<LifecycleNode>("Child", events);

    LifecycleNode* child_pointer = child.get();
    expect(root->add_child(std::move(child)) == child_pointer,
           "node accepts detached child");
    expect(root->get_child_count() == 1, "node stores child ownership");
    expect(child_pointer->get_path() == "/Root/Child", "node path follows hierarchy");
    expect(child_pointer->set_name("Child"), "node accepts unchanged name");

    auto duplicate = std::make_unique<LifecycleNode>("Child", events);
    expect(root->add_child(std::move(duplicate)) == nullptr,
           "node rejects duplicate sibling names");

    nle::SceneTree tree;
    expect(tree.set_root(std::move(root)), "scene tree accepts detached root");
    expect(tree.initialize(), "scene tree initializes");
    expect(tree.is_initialized(), "scene tree reports initialized");

    const std::vector<std::string> lifecycle_expected{
        "Root.enter", "Child.enter", "Child.ready", "Root.ready"
    };
    expect(events == lifecycle_expected, "scene tree enter and ready ordering");

    events.clear();
    tree.process_frame(1.0 / 60.0);
    expect(events == std::vector<std::string>{"Root.process", "Child.process"},
           "scene tree processes parent before child");

    events.clear();
    tree.physics_frame(1.0 / 60.0);
    expect(events == std::vector<std::string>{"Root.physics", "Child.physics"},
           "scene tree physics processes parent before child");

    events.clear();
    std::unique_ptr<nle::Node> removed = tree.get_root()->remove_child(child_pointer);
    expect(removed.get() == child_pointer, "node returns removed child ownership");
    expect(events == std::vector<std::string>{"Child.exit"},
           "removed node exits the scene tree");

    events.clear();
    tree.shutdown();
    expect(events == std::vector<std::string>{"Root.exit"},
           "scene tree shuts down root lifecycle");
    expect(!tree.is_initialized(), "scene tree reports shutdown");
}

void test_error_codes() {
    using nle::ErrorCode;

    expect(nle::error_code_name(ErrorCode::Ok) == "Ok", "Ok error code name");
    expect(nle::error_code_name(ErrorCode::InvalidParameter) == "InvalidParameter",
           "InvalidParameter error code name");
    expect(nle::error_code_name(ErrorCode::PlatformError) == "PlatformError",
           "PlatformError error code name");
}

void test_class_db() {
    nle::ClassDB& classes = nle::ClassDB::instance();

    expect(classes.class_exists(nle::StringName("Object")), "Object base class is registered");
    expect(classes.register_class<TestObject>("TestObject"), "class registration succeeds");
    expect(!classes.register_class<TestObject>("TestObject"), "duplicate registration fails");
    expect(classes.is_parent_class(
        nle::StringName("TestObject"),
        nle::StringName("Object")
    ), "class hierarchy resolves parent");
    const std::vector<nle::StringName> registered_classes = classes.get_class_list();
    expect(std::is_sorted(registered_classes.begin(), registered_classes.end()),
           "class database returns deterministic sorted class names");

    std::unique_ptr<nle::Object> instance =
        classes.instantiate(nle::StringName("TestObject"));
    expect(instance != nullptr, "registered class can be instantiated");
    expect(std::string_view(instance->get_class_name()) == "TestObject",
           "instantiated object reports its class");

    expect(classes.instantiate(nle::StringName("MissingClass")) == nullptr,
           "unknown class cannot be instantiated");
}

void test_object_registry() {
    auto* object = new TestObject();
    const nle::ObjectID id = object->get_instance_id();

    expect(id.is_valid(), "object receives a valid identifier");
    expect(nle::ObjectDB::get_instance(id) == object, "object is registered");

    delete object;

    expect(nle::ObjectDB::get_instance(id) == nullptr, "destroyed object is unregistered");
}

void test_reference_counting() {
    int destruction_count = 0;
    nle::ObjectID id;

    {
        nle::Ref<TestResource> resource = nle::make_ref<TestResource>(&destruction_count);
        id = resource->get_instance_id();

        expect(resource->get_reference_count() == 1, "new Ref owns one reference");

        {
            nle::Ref<TestResource> copy = resource;
            expect(resource->get_reference_count() == 2, "copy increments reference count");
            expect(copy.get() == resource.get(), "copied Ref points to same object");
        }

        expect(resource->get_reference_count() == 1, "destroyed copy releases its reference");
        expect(destruction_count == 0, "resource remains alive while referenced");
    }

    expect(destruction_count == 1, "last reference destroys resource");
    expect(!nle::ObjectDB::instance_exists(id), "released resource leaves object registry");
}

void test_file_access() {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "noctislaby-file-access-test.bin";
    std::error_code filesystem_error;
    std::filesystem::remove(path, filesystem_error);

    const std::array<std::byte, 4> source{
        std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}
    };

    nle::ErrorCode error = nle::ErrorCode::Failed;
    {
        auto file = nle::FileAccess::open(path, nle::FileMode::Write, &error);
        expect(file != nullptr, "file opens for writing");
        expect(error == nle::ErrorCode::Ok, "file open clears error state");
        expect(file->write(source), "file writes binary bytes");
        file->flush();
        expect(file->get_length() == source.size(), "file reports byte length");
    }

    {
        auto file = nle::FileAccess::open(path, nle::FileMode::Read, &error);
        expect(file != nullptr, "file opens for reading");

        std::array<std::byte, 4> destination{};
        expect(file->read(destination) == destination.size(), "file reads binary bytes");
        expect(destination == source, "file contents round trip");
        expect(file->get_position() == 4, "file reports current position");

        expect(file->seek(1), "file seeks to byte offset");
        std::array<std::byte, 2> partial{};
        expect(file->read(partial) == partial.size(), "file reads after seeking");
        expect(partial[0] == source[1] && partial[1] == source[2],
               "seek reads expected bytes");
    }

    std::filesystem::remove(path, filesystem_error);
}

void test_resource_saver() {
    nle::ResourceSaver saver;
    auto format_saver = std::make_shared<TestFormatSaver>();
    nle::Ref<nle::Resource> resource = nle::make_ref<nle::Resource>();

    expect(saver.add_format_saver(format_saver), "resource format saver registers");
    expect(!saver.add_format_saver(format_saver), "duplicate format saver is rejected");
    expect(saver.save("res://materials/test.NLERES", resource) == nle::ErrorCode::Ok,
           "resource saver dispatches by extension");
    expect(format_saver->save_count == 1, "format saver receives resource");
    expect(format_saver->saved_path == "res://materials/test.NLERES",
           "format saver receives destination path");
    expect(saver.save("res://materials/test.unknown", resource) == nle::ErrorCode::NotFound,
           "unsupported save extension fails");
    expect(saver.save("res://materials/test.nleres", {}) ==
           nle::ErrorCode::InvalidParameter, "null resource cannot be saved");
}

void test_resource_loader() {
    nle::ResourceLoader loader;
    auto format_loader = std::make_shared<TestFormatLoader>();

    expect(loader.add_format_loader(format_loader), "resource format loader registers");
    expect(!loader.add_format_loader(format_loader), "duplicate format loader is rejected");

    nle::ErrorCode error = nle::ErrorCode::Failed;
    nle::Ref<nle::Resource> first = loader.load("res://materials/test.NLERES", &error);

    expect(first.get() != nullptr, "resource loader dispatches by extension");
    expect(error == nle::ErrorCode::Ok, "resource load reports success");
    expect(first->get_path() == "res://materials/test.NLERES",
           "resource loader assigns source path");

    nle::Ref<nle::Resource> second = loader.load("res://materials//folder/../test.NLERES", &error);
    expect(second.get() == first.get(), "resource loader returns cached instance");
    expect(format_loader->load_count == 1, "resource cache avoids duplicate format loads");

    expect(!loader.load("res://materials/unknown.bin", &error),
           "unsupported resource extension fails");
    expect(error == nle::ErrorCode::NotFound, "unsupported extension reports NotFound");

    expect(!loader.load("", &error), "empty resource path fails");
    expect(error == nle::ErrorCode::InvalidParameter,
           "empty resource path reports InvalidParameter");
}

void test_concurrent_resource_loading() {
    constexpr std::size_t ThreadCount = 8;
    nle::ResourceLoader loader;
    auto format_loader = std::make_shared<ConcurrentTestFormatLoader>();
    expect(loader.add_format_loader(format_loader),
           "concurrent resource loader registers format handler");

    std::barrier start_gate(static_cast<std::ptrdiff_t>(ThreadCount));
    std::array<nle::Ref<nle::Resource>, ThreadCount> results;
    std::array<nle::ErrorCode, ThreadCount> errors{};
    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (std::size_t index = 0; index < ThreadCount; ++index) {
        workers.emplace_back([&, index] {
            start_gate.arrive_and_wait();
            results[index] = loader.load("res://concurrent/shared.nleres", &errors[index]);
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    expect(format_loader->load_count.load() == 1,
           "concurrent requests share one in-flight resource load");
    expect(std::all_of(results.begin(), results.end(), [](const auto& resource) {
        return static_cast<bool>(resource);
    }), "all concurrent resource requests receive a resource");
    expect(std::all_of(errors.begin(), errors.end(), [](const auto error) {
        return error == nle::ErrorCode::Ok;
    }), "all concurrent resource requests receive successful status");
    expect(std::all_of(results.begin(), results.end(), [&results](const auto& resource) {
        return resource.get() == results.front().get();
    }), "concurrent resource requests receive the same cached instance");
}

void test_project() {
    const std::filesystem::path project_root =
        std::filesystem::temp_directory_path() / "noctislaby-project-create-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(project_root, filesystem_error);

    expect(nle::Project::create(project_root, "NoctisLaby Sample") == nle::ErrorCode::Ok,
           "project manager creates a project directory");
    expect(nle::Project::create(project_root, "Duplicate") == nle::ErrorCode::AlreadyExists,
           "project manager refuses to overwrite an existing project");

    nle::Project project;
    expect(nle::Project::load(project_root, project) == nle::ErrorCode::Ok,
           "project manager opens a project directory");
    expect(project.get_name() == "NoctisLaby Sample",
           "project manager loads project identity");
    expect(project.get_file_path().filename() == nle::Project::ProjectFileName,
           "project manager resolves the project descriptor");
    expect(project.get_root_path() == std::filesystem::absolute(project_root).lexically_normal(),
           "project manager resolves the project root");
    expect(project.get_config().get_value("rendering", "renderer") ==
           std::optional<std::string>("null"),
           "new projects receive default renderer settings");

    expect(project.get_config().set_value(
        "application", "main_scene", "res://scenes/./main.nlescene"
    ), "project settings accept a main scene path");
    expect(project.save() == nle::ErrorCode::Ok,
           "project manager saves modified project settings");

    nle::Project reopened;
    expect(nle::Project::load(project_root, reopened) == nle::ErrorCode::Ok,
           "project manager reopens saved settings");
    expect(reopened.get_config().get_value("application", "main_scene") ==
           std::optional<std::string>("res://scenes/main.nlescene"),
           "project manager canonicalizes the configured main scene");

    const std::filesystem::path invalid_root =
        std::filesystem::temp_directory_path() / "noctislaby-project-invalid-version";
    std::filesystem::remove_all(invalid_root, filesystem_error);
    expect(nle::Project::create(invalid_root, "Invalid Version Test") == nle::ErrorCode::Ok,
           "project manager creates invalid-version fixture");
    nle::ProjectConfig invalid_config;
    expect(invalid_config.set_value("application", "name", "Invalid Version Test"),
           "project config sets invalid-version fixture name");
    expect(invalid_config.set_value("application", "config_version", "99"),
           "project config sets unsupported version");
    expect(invalid_config.save(invalid_root / nle::Project::ProjectFileName) ==
           nle::ErrorCode::Ok, "project config writes unsupported version fixture");
    expect(nle::Project::load(invalid_root, reopened) == nle::ErrorCode::NotImplemented,
           "project manager rejects unsupported project versions");

    std::filesystem::remove_all(project_root, filesystem_error);
    std::filesystem::remove_all(invalid_root, filesystem_error);
}

void test_project_config() {
    nle::ProjectConfig config;
    expect(config.set_value("application", "name", "NoctisLaby Test"),
           "project config stores string values");
    expect(config.set_value("application", "main_scene", "res://scenes/main.nlescene"),
           "project config stores resource paths");
    expect(config.set_value("rendering", "vsync", "true"),
           "project config stores boolean values");
    expect(config.set_value("physics", "ticks_per_second", "120"),
           "project config stores integer values");
    expect(config.set_value("application", "description", "A \"quoted\" line\nsecond line"),
           "project config stores escaped text");
    expect(!config.set_value("", "invalid", "value"),
           "project config rejects empty section names");

    bool vsync = false;
    std::int64_t ticks = 0;
    expect(config.get_bool("rendering", "vsync", vsync) && vsync,
           "project config parses boolean values");
    expect(config.get_int("physics", "ticks_per_second", ticks) && ticks == 120,
           "project config parses integer values");
    expect(!config.get_bool("physics", "ticks_per_second", vsync),
           "project config rejects invalid boolean conversions");

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "noctislaby-project-test.nleproject";
    const std::filesystem::path invalid_path =
        std::filesystem::temp_directory_path() / "noctislaby-project-invalid.nleproject";
    std::error_code filesystem_error;
    std::filesystem::remove(path, filesystem_error);
    std::filesystem::remove(invalid_path, filesystem_error);

    expect(config.save(path) == nle::ErrorCode::Ok,
           "project config saves deterministic project files");

    nle::ProjectConfig restored;
    expect(restored.set_value("temporary", "key", "old"),
           "project config prepares transactional load test");
    expect(restored.load(path) == nle::ErrorCode::Ok,
           "project config loads project files");
    expect(restored.get_value("application", "name") ==
           std::optional<std::string>("NoctisLaby Test"),
           "project config restores string values");
    expect(restored.get_value("application", "description") ==
           std::optional<std::string>("A \"quoted\" line\nsecond line"),
           "project config restores escaped text");
    expect(!restored.has_value("temporary", "key"),
           "project config replaces previous state after successful load");

    {
        auto file = nle::FileAccess::open(invalid_path, nle::FileMode::Write);
        expect(static_cast<bool>(file), "project config opens invalid fixture");
        const std::string invalid = "[application]\nname = \"unterminated\n";
        const auto bytes = std::as_bytes(std::span(invalid.data(), invalid.size()));
        expect(file->write(bytes), "project config writes invalid fixture");
    }
    expect(restored.load(invalid_path) == nle::ErrorCode::InvalidParameter,
           "project config rejects malformed values");
    expect(restored.get_value("application", "name") ==
           std::optional<std::string>("NoctisLaby Test"),
           "failed project config load preserves existing state");

    std::filesystem::remove(path, filesystem_error);
    std::filesystem::remove(invalid_path, filesystem_error);
}

void test_resources() {
    nle::Ref<nle::Resource> resource = nle::make_ref<TestTexture>();
    int changed_count = 0;

    const nle::ConnectionID connection = resource->changed_signal().connect(
        [&changed_count] { ++changed_count; }
    );

    resource->set_path("res://textures/stone.png");
    resource->set_name("Stone");
    resource->set_uid(nle::ResourceUID{42});

    expect(std::string_view(resource->get_class_name()) == "Resource",
           "resource reports its engine class");
    expect(resource->get_path() == "res://textures/stone.png", "resource path");
    expect(resource->get_name() == "Stone", "resource name");
    expect(resource->get_uid() == nle::ResourceUID{42}, "resource UID");
    expect(changed_count == 3, "resource metadata changes emit notifications");

    resource->set_path("res://textures/stone.png");
    expect(changed_count == 3, "unchanged resource path does not emit");

    resource->set_local_to_scene(true);
    expect(resource->is_local_to_scene(), "resource local-to-scene flag");

    nle::ResourceCache cache;
    expect(cache.insert(resource), "resource cache accepts path-backed resource");
    expect(cache.size() == 1, "resource cache tracks entries");
    expect(cache.contains("res://textures/stone.png"), "resource cache finds path");

    nle::Ref<nle::Resource> loaded = cache.get("res://textures/stone.png");
    expect(loaded.get() == resource.get(), "resource cache returns shared instance");
    expect(resource->get_reference_count() == 3, "cache and loaded handle retain resource");

    expect(cache.remove("res://textures/stone.png"), "resource cache removes entry");
    loaded.reset();
    expect(resource->get_reference_count() == 1, "removed cache entry releases ownership");
    expect(!cache.remove("res://textures/stone.png"), "resource cache rejects missing entry");

    expect(cache.insert(resource), "resource can be cached again");
    cache.clear();
    expect(cache.size() == 0, "resource cache clears all entries");

    expect(resource->changed_signal().disconnect(connection),
           "resource change connection can be disconnected");
}

void test_resource_paths() {
    std::string normalized;
    expect(nle::ResourcePath::normalize(
        "RES://textures\\\\stone/./high/../stone.png", normalized
    ) == nle::ErrorCode::Ok, "resource path normalizes separators and dot segments");
    expect(normalized == "res://textures/stone/stone.png",
           "resource path produces canonical URI");
    expect(nle::ResourcePath::normalize(
        "user://saves//slot-1/../slot-2.dat", normalized
    ) == nle::ErrorCode::Ok && normalized == "user://saves/slot-2.dat",
           "user resource path normalizes repeated separators");
    expect(nle::ResourcePath::normalize("res://../../outside", normalized) ==
           nle::ErrorCode::InvalidParameter,
           "resource path rejects traversal above its scheme root");
    expect(nle::ResourcePath::normalize("file:///etc/passwd", normalized) ==
           nle::ErrorCode::InvalidParameter,
           "resource path rejects unsupported schemes");
    expect(nle::ResourcePath::normalize("res://", normalized) ==
           nle::ErrorCode::InvalidParameter,
           "resource path rejects empty resource names");
}

void test_resource_uid_registry() {
    nle::ResourceUIDRegistry registry;
    const nle::ResourceUID texture_uid{0x1020304050607080ULL};
    const nle::ResourceUID scene_uid{0x8877665544332211ULL};

    expect(registry.register_uid(texture_uid, "res://textures/stone.nleres"),
           "UID registry registers a resource path");
    expect(registry.register_uid(scene_uid, "res://scenes/world.nlescene"),
           "UID registry registers a second resource path");
    expect(!registry.register_uid({}, "res://invalid.nleres"),
           "UID registry rejects the invalid UID");
    expect(!registry.register_uid(texture_uid, "res://textures/other.nleres"),
           "UID registry rejects UID collisions");
    expect(!registry.register_uid(nle::ResourceUID{99}, "res://textures//stone/../stone.nleres"),
           "UID registry rejects path collisions");
    expect(registry.get_uid("res://textures/stone.nleres") == texture_uid,
           "UID registry resolves path to UID");
    expect(registry.get_path(scene_uid) == "res://scenes/world.nlescene",
           "UID registry resolves UID to path");
    expect(registry.size() == 2, "UID registry tracks registered resources");

    const std::filesystem::path file_path =
        std::filesystem::temp_directory_path() / "noctislaby-resource-uids-test.bin";
    std::error_code filesystem_error;
    std::filesystem::remove(file_path, filesystem_error);
    expect(registry.save(file_path), "UID registry saves its versioned database");

    nle::ResourceUIDRegistry restored;
    expect(restored.register_uid(nle::ResourceUID{7}, "res://temporary.nleres"),
           "UID registry prepares transactional load test");
    expect(restored.load(file_path), "UID registry loads its database");
    expect(restored.size() == 2, "UID registry replaces old state after load");
    expect(restored.get_uid("res://textures/stone.nleres") == texture_uid,
           "UID registry preserves UID mappings across save and load");
    expect(restored.get_path(scene_uid) == "res://scenes/world.nlescene",
           "UID registry preserves paths across save and load");

    const std::filesystem::path corrupt_path =
        std::filesystem::temp_directory_path() / "noctislaby-resource-uids-corrupt.bin";
    {
        auto corrupt = nle::FileAccess::open(corrupt_path, nle::FileMode::Write);
        expect(static_cast<bool>(corrupt), "UID registry opens corrupt fixture");
        const std::array<std::byte, 4> invalid_data{
            std::byte{'B'}, std::byte{'A'}, std::byte{'D'}, std::byte{'!'}
        };
        expect(corrupt->write(invalid_data), "UID registry writes corrupt fixture");
    }
    expect(!restored.load(corrupt_path), "UID registry rejects corrupt databases");
    expect(restored.size() == 2 &&
           restored.get_uid("res://textures/stone.nleres") == texture_uid,
           "failed UID database load leaves existing state unchanged");

    expect(restored.unregister_uid(texture_uid), "UID registry unregisters a resource");
    expect(!restored.unregister_uid(texture_uid), "UID registry rejects duplicate removal");
    expect(!restored.get_uid("res://textures/stone.nleres").is_valid(),
           "UID registry removes both lookup directions");

    std::filesystem::remove(file_path, filesystem_error);
    std::filesystem::remove(corrupt_path, filesystem_error);
}

void test_signals() {
    nle::Signal<int> signal;
    int total = 0;

    const nle::ConnectionID first = signal.connect(
        [&total](const int value) { total += value; }
    );
    const nle::ConnectionID second = signal.connect(
        [&total](const int value) { total += value * 2; }
    );

    expect(first != 0 && second != 0, "signal connections receive identifiers");
    expect(signal.connection_count() == 2, "signal tracks active connections");

    signal.emit(3);
    expect(total == 9, "signal invokes connected callbacks");

    expect(signal.disconnect(first), "signal disconnects existing callback");
    expect(!signal.disconnect(first), "signal rejects duplicate disconnection");

    signal.emit(2);
    expect(total == 13, "disconnected callback is not invoked again");

    nle::Signal<> ordered_signal;
    std::vector<int> callback_order;
    expect(ordered_signal.connect([&callback_order] { callback_order.push_back(1); }) != 0,
           "ordered signal registers first callback");
    expect(ordered_signal.connect([&callback_order] { callback_order.push_back(2); }) != 0,
           "ordered signal registers second callback");
    ordered_signal.emit();
    expect(callback_order == std::vector<int>{1, 2},
           "signal callbacks execute in connection order");

    nle::ConnectionID self_connection = 0;
    int self_calls = 0;
    self_connection = signal.connect([&](const int) {
        ++self_calls;
        static_cast<void>(signal.disconnect(self_connection));
    });

    signal.emit(1);
    signal.emit(1);
    expect(self_calls == 1, "callbacks can disconnect themselves during emission");
}

void test_string_names() {
    const nle::StringName first("RenderingServer");
    const nle::StringName second("RenderingServer");
    const nle::StringName other("PhysicsServer");

    expect(first == second, "equal names compare equal");
    expect(first.hash() == second.hash(), "equal names have equal hashes");
    expect(first != other, "different names compare different");
    expect(first.view() == "RenderingServer", "name exposes its string view");
    expect(nle::StringName{}.empty(), "default name is empty");

    std::unordered_map<nle::StringName, int> names;
    names.emplace(first, 42);
    expect(names.at(second) == 42, "string names work as hash map keys");
}

void test_variants() {
    const nle::Variant empty;
    const nle::Variant boolean(true);
    const nle::Variant integer(std::int64_t{123});
    const nle::Variant floating(nle::real_t{0.5F});
    const nle::Variant text("NoctisLaby");
    const nle::Variant vector(nle::Vector3{1, 2, 3});

    expect(empty.is_nil(), "default Variant is nil");
    expect(empty.get_type() == nle::Variant::Type::Nil, "nil Variant type");
    expect(boolean.get_type() == nle::Variant::Type::Bool, "boolean Variant type");
    expect(*boolean.get_if<bool>(), "boolean Variant value");
    expect(integer.get_type() == nle::Variant::Type::Int, "integer Variant type");
    expect(*integer.get_if<std::int64_t>() == 123, "integer Variant value");
    expect(floating.get_type() == nle::Variant::Type::Float, "floating Variant type");
    expect(text.to_string() == "NoctisLaby", "string Variant formatting");
    expect(vector.to_string() == "(1, 2, 3)", "Vector3 Variant formatting");
    expect(nle::Variant(std::int32_t{123}) == integer, "integer widths share Variant type");
}

void test_color() {
    const nle::Color srgb{0.5F, 0.25F, 0.75F, 0.8F};
    const nle::Color linear = nle::Color::from_srgb(srgb);
    const nle::Color restored = linear.to_srgb();

    expect(std::abs(restored.r - srgb.r) < nle::real_t{0.0001F} &&
           std::abs(restored.g - srgb.g) < nle::real_t{0.0001F} &&
           std::abs(restored.b - srgb.b) < nle::real_t{0.0001F},
           "sRGB and linear color conversion round trip");
    expect(restored.a == srgb.a, "color conversion preserves alpha");

    const nle::Color clamped = nle::Color{2, -1, 0.5F, 1.5F}.clamped();
    expect(clamped == nle::Color{1, 0, 0.5F, 1}, "color clamping");
    expect(nle::Color{1, 0.5F, 0.25F, 0.5F}.premultiplied() ==
           nle::Color{0.5F, 0.25F, 0.125F, 0.5F}, "premultiplied alpha");
}

void test_plane() {
    const nle::Plane plane = nle::Plane::from_point_normal({0, 2, 0}, {0, 2, 0});
    expect(plane.has_point({0, 2, 0}), "plane contains defining point");

    const nle::Vector3 projected = plane.project({1, 4, 3});
    expect(std::abs(projected.x - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(projected.y - nle::real_t{2}) < nle::real_t{0.0001F} &&
           std::abs(projected.z - nle::real_t{3}) < nle::real_t{0.0001F},
           "plane projection handles non-unit normal");

    nle::Vector3 hit;
    expect(plane.intersect_ray({0, 0, 0}, {0, 1, 0}, hit),
           "ray intersects plane");
    expect(hit == nle::Vector3{0, 2, 0}, "ray-plane intersection point");
    expect(!plane.intersect_ray({0, 3, 0}, {0, 1, 0}, hit),
           "ray pointing away from plane does not intersect");
}

void test_transform2d() {
    const nle::Transform2D transform{
        {2, 0},
        {0, 3},
        {10, 20}
    };

    const nle::Vector2 world_point = transform.xform({1, 1});
    expect(world_point == nle::Vector2{12, 23}, "Transform2D applies basis and origin");

    nle::Transform2D inverse;
    expect(transform.try_affine_inverse(inverse), "Transform2D has affine inverse");

    const nle::Vector2 local_point = inverse.xform(world_point);
    expect(std::abs(local_point.x - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(local_point.y - nle::real_t{1}) < nle::real_t{0.0001F},
           "Transform2D inverse restores local point");

    const nle::Transform2D singular{{1, 0}, {2, 0}, {0, 0}};
    expect(!singular.try_affine_inverse(inverse), "singular Transform2D has no inverse");
}

void test_transforms() {
    const nle::real_t half_pi = std::numbers::pi_v<nle::real_t> / nle::real_t{2};
    const nle::Quaternion rotation =
        nle::Quaternion::from_axis_angle({0, 0, 1}, half_pi);
    const nle::Basis rotated_basis = nle::Basis::from_quaternion(rotation);
    const nle::Vector3 rotated_axis = rotated_basis.xform({1, 0, 0});

    expect(std::abs(rotated_axis.x) < nle::real_t{0.0001F},
           "Basis quaternion conversion x");
    expect(std::abs(rotated_axis.y - nle::real_t{1}) < nle::real_t{0.0001F},
           "Basis quaternion conversion y");

    const nle::Basis scale{
        {2, 0, 0},
        {0, 3, 0},
        {0, 0, 4}
    };
    expect(scale.determinant() == nle::real_t{24}, "Basis determinant");

    nle::Basis inverse_basis;
    expect(scale.try_inverse(inverse_basis), "non-singular Basis has inverse");
    const nle::Vector3 restored = inverse_basis.xform(scale.xform({1, 1, 1}));
    expect(std::abs(restored.x - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(restored.y - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(restored.z - nle::real_t{1}) < nle::real_t{0.0001F},
           "Basis inverse restores vector");

    nle::Basis singular{{1, 0, 0}, {2, 0, 0}, {0, 0, 1}};
    expect(!singular.try_inverse(inverse_basis), "singular Basis has no inverse");

    const nle::Transform3D transform{scale, {10, 20, 30}};
    const nle::Vector3 world_point = transform.xform({1, 1, 1});
    expect(world_point == nle::Vector3{12, 23, 34}, "Transform3D applies basis and origin");

    nle::Transform3D inverse_transform;
    expect(transform.try_affine_inverse(inverse_transform), "Transform3D has affine inverse");
    const nle::Vector3 local_point = inverse_transform.xform(world_point);
    expect(std::abs(local_point.x - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(local_point.y - nle::real_t{1}) < nle::real_t{0.0001F} &&
           std::abs(local_point.z - nle::real_t{1}) < nle::real_t{0.0001F},
           "Transform3D inverse restores local point");
}

void test_quaternions() {
    const nle::real_t half_pi = std::numbers::pi_v<nle::real_t> / nle::real_t{2};
    const nle::Quaternion rotation =
        nle::Quaternion::from_axis_angle({0, 0, 1}, half_pi);

    const nle::Vector3 rotated = rotation.xform({1, 0, 0});
    expect(std::abs(rotated.x) < nle::real_t{0.0001F},
           "quaternion rotates x axis away from x");
    expect(std::abs(rotated.y - nle::real_t{1}) < nle::real_t{0.0001F},
           "quaternion rotates x axis into y");

    const nle::Quaternion halfway = nle::Quaternion::slerp(
        nle::Quaternion{},
        rotation,
        nle::real_t{0.5F}
    );
    const nle::Vector3 halfway_vector = halfway.xform({1, 0, 0});
    expect(std::abs(halfway_vector.x - nle::real_t{0.7071F}) < nle::real_t{0.001F},
           "quaternion slerp interpolates rotation");
    expect(std::abs(halfway_vector.y - nle::real_t{0.7071F}) < nle::real_t{0.001F},
           "quaternion slerp interpolates rotation axis");

    expect(nle::Quaternion::from_axis_angle({}, half_pi) == nle::Quaternion{},
           "zero axis produces identity rotation");
}

void test_geometry() {
    const nle::Rect2 first{{0, 0}, {10, 10}};
    const nle::Rect2 second{{5, 5}, {10, 10}};

    expect(first.has_point({9, 9}), "Rect2 contains interior point");
    expect(!first.has_point({10, 10}), "Rect2 excludes far edge");
    expect(first.intersects(second), "Rect2 intersection detection");
    expect(first.intersection(second) == nle::Rect2{{5, 5}, {5, 5}},
           "Rect2 intersection bounds");
    expect(first.merge(second) == nle::Rect2{{0, 0}, {15, 15}},
           "Rect2 merged bounds");

    const nle::AABB box_a{{0, 0, 0}, {4, 4, 4}};
    const nle::AABB box_b{{2, 2, 2}, {4, 4, 4}};

    expect(box_a.has_point({1, 1, 1}), "AABB contains interior point");
    expect(box_a.intersects(box_b), "AABB intersection detection");
    expect(box_a.intersection(box_b) == nle::AABB{{2, 2, 2}, {2, 2, 2}},
           "AABB intersection bounds");
    expect(box_a.merge(box_b) == nle::AABB{{0, 0, 0}, {6, 6, 6}},
           "AABB merged bounds");
}

void test_vectors() {
    const nle::Vector2 horizontal{3, 4};
    expect(horizontal.length() == nle::real_t{5}, "Vector2 length");
    expect(horizontal.dot({1, 0}) == nle::real_t{3}, "Vector2 dot product");
    expect(nle::Vector2{1, 0}.cross({0, 1}) == nle::real_t{1},
           "Vector2 scalar cross product");

    const nle::Vector2 normalized = horizontal.normalized();
    expect(std::abs(normalized.x - nle::real_t{0.6F}) < nle::real_t{0.0001F},
           "Vector2 normalized x");
    expect(std::abs(normalized.y - nle::real_t{0.8F}) < nle::real_t{0.0001F},
           "Vector2 normalized y");

    const nle::Vector3 right{1, 0, 0};
    const nle::Vector3 up{0, 1, 0};
    expect(right.cross(up) == nle::Vector3{0, 0, 1}, "Vector3 cross product");
    expect(right.dot(up) == nle::real_t{0}, "Vector3 orthogonal dot product");
    expect(nle::Vector3{}.normalized().is_zero(), "zero Vector3 remains zero");

    const nle::Vector4 a{1, 2, 3, 4};
    const nle::Vector4 b{2, 2, 2, 2};
    expect(a.dot(b) == nle::real_t{20}, "Vector4 dot product");
}

void test_memory() {
    const nle::MemoryStats before = nle::Memory::get_stats();
    auto* memory = static_cast<unsigned char*>(nle::Memory::allocate(64, 64));

    expect(reinterpret_cast<std::uintptr_t>(memory) % 64 == 0,
           "allocation satisfies requested alignment");
    expect(nle::Memory::get_stats().active_allocations == before.active_allocations + 1,
           "allocation increments active allocation count");
    expect(nle::Memory::get_stats().allocated_bytes == before.allocated_bytes + 64,
           "allocation increments tracked byte count");

    for (std::size_t index = 0; index < 64; ++index) {
        memory[index] = static_cast<unsigned char>(index);
    }

    memory = static_cast<unsigned char*>(nle::Memory::reallocate(memory, 64, 128, 64));
    for (std::size_t index = 0; index < 64; ++index) {
        expect(memory[index] == static_cast<unsigned char>(index),
               "reallocation preserves existing bytes");
    }

    nle::Memory::deallocate(memory, 128, 64);

    const nle::MemoryStats after = nle::Memory::get_stats();
    expect(after.active_allocations == before.active_allocations,
           "deallocation restores active allocation count");
    expect(after.allocated_bytes == before.allocated_bytes,
           "deallocation restores tracked byte count");
}

} // namespace

int main() {
    test_physics_server_2d();
    test_input_server();
    test_input_map();
    test_render_graph();
    test_rendering_device();
    test_spatial_nodes();
    test_scene_tree();
    test_main_loop();
    test_error_codes();
    test_class_db();
    test_object_registry();
    test_reference_counting();
    test_file_access();
    test_resource_saver();
    test_resource_loader();
    test_concurrent_resource_loading();
    test_resources();
    test_project_config();
    test_project();
    test_resource_uid_registry();
    test_resource_paths();
    test_signals();
    test_string_names();
    test_variants();
    test_quaternions();
    test_color();
    test_plane();
    test_transform2d();
    test_transforms();
    test_geometry();
    test_vectors();
    test_memory();

    std::cout << "All NoctisLaby core tests passed.\n";
    return EXIT_SUCCESS;
}
