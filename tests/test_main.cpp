#include "error/error.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "memory/memory.h"
#include "object/object.h"
#include "object/object_db.h"
#include "object/ref_counted.h"
#include "string/string_name.h"

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

class TestObject final : public nle::Object {};

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
    test_object_registry();
    test_reference_counting();
    test_string_names();
    test_vectors();
    test_memory();

    std::cout << "All NoctisLaby core tests passed.\n";
    return EXIT_SUCCESS;
}
