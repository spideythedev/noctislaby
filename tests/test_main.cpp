#include "error/error.h"
#include "io/resource.h"
#include "object/class_db.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "memory/memory.h"
#include "object/object.h"
#include "object/object_db.h"
#include "object/ref_counted.h"
#include "object/signal.h"
#include "string/string_name.h"
#include "variant/variant.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
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

void test_resources() {
    nle::Ref<nle::Resource> resource = nle::make_ref<nle::Resource>();
    int changed_count = 0;

    const nle::ConnectionID connection = resource->changed_signal().connect(
        [&changed_count] { ++changed_count; }
    );

    resource->set_path("res://textures/stone.png");
    resource->set_name("Stone");
    resource->set_uid(nle::ResourceUID{42});

    expect(resource->get_class_name() == std::string_view("Resource"),
           "resource reports its engine class");
    expect(resource->get_path() == "res://textures/stone.png", "resource path");
    expect(resource->get_name() == "Stone", "resource name");
    expect(resource->get_uid() == nle::ResourceUID{42}, "resource UID");
    expect(changed_count == 3, "resource metadata changes emit notifications");

    resource->set_path("res://textures/stone.png");
    expect(changed_count == 3, "unchanged resource path does not emit");

    resource->set_local_to_scene(true);
    expect(resource->is_local_to_scene(), "resource local-to-scene flag");

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

void test_vectors() {
    const nle::Vector2 horizontal{3, 4};
    expect(horizontal.length() == nle::real_t{5}, "Vector2 length");
    expect(horizontal.dot({1, 0}) == nle::real_t{3}, "Vector2 dot product");
    expect(nle::Vector2{1, 0}.cross({0, 1}) == nle::real_t{1},
           "Vector2 scalar cross product");

    const nle::Vector2 normalized = horizontal.normalized();
    expect(std::abs(normalized.x - nle::real_t{0.6F}) < nle::real_t{0.0001F},
           "Vector2 normalized x");
    expect(std::abs(normalized.y - nle::real_t{0.8F}) < nle::real_t{0.0001},
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
    test_resources();
    test_signals();
    test_string_names();
    test_variants();
    test_vectors();
    test_memory();

    std::cout << "All NoctisLaby core tests passed.\n";
    return EXIT_SUCCESS;
}
