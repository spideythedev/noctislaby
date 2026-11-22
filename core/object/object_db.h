#pragma once

#include "object_id.h"

#include <mutex>
#include <unordered_map>

namespace nle {

class Object;

class ObjectDB final {
public:
    [[nodiscard]] static Object* get_instance(ObjectID id) noexcept;
    [[nodiscard]] static bool instance_exists(ObjectID id) noexcept;

private:
    friend class Object;

    ObjectDB() = default;

    [[nodiscard]] static ObjectID add(Object* object);
    static void remove(ObjectID id, const Object* expected) noexcept;
    [[nodiscard]] static ObjectDB& instance();

    std::mutex mutex_;
    std::unordered_map<ObjectID, Object*, ObjectIDHash> objects_;
    std::uint64_t next_id_ = 1;
};

} // namespace nle
