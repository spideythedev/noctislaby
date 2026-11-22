#include "error/error.h"
#include "io/file_access.h"
#include "io/resource.h"
#include "io/resource_cache.h"
#include "io/resource_loader.h"
#include "io/resource_saver.h"
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
#include "string/string_name.h"
#include "variant/variant.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

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

    nle::Ref<nle::Resource> second = loader.load("res://materials/test.NLERES", &error);
    expect(second.get() == first.get(), "resource loader returns cached instance");
    expect(format_loader->load_count == 1, "resource cache avoids duplicate format loads");

    expect(!loader.load("res://materials/unknown.bin", &error),
           "unsupported resource extension fails");
    expect(error == nle::ErrorCode::NotFound, "unsupported extension reports NotFound");

    expect(!loader.load("", &error), "empty resource path fails");
    expect(error == nle::ErrorCode::InvalidParameter,
           "empty resource path reports InvalidParameter");
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
    test_error_codes();
    test_class_db();
    test_object_registry();
    test_reference_counting();
    test_file_access();
    test_resource_saver();
    test_resource_loader();
    test_resources();
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
