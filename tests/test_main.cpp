#include "error/error.h"
#include "object/object.h"
#include "object/object_db.h"
#include "object/ref_counted.h"
#include "string/string_name.h"

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

} // namespace

int main() {
    test_error_codes();
    test_object_registry();
    test_reference_counting();
    test_string_names();

    std::cout << "All NoctisLaby core tests passed.\n";
    return EXIT_SUCCESS;
}
