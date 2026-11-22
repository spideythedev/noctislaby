#include "object_db.h"

#include "object.h"

#include <limits>
#include <stdexcept>

namespace nle {

ObjectDB& ObjectDB::instance() {
    static ObjectDB database;
    return database;
}

ObjectID ObjectDB::add(Object* object) {
    ObjectDB& database = instance();
    std::lock_guard lock(database.mutex_);

    if (database.next_id_ == 0 ||
        database.next_id_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("NoctisLaby object identifier space exhausted.");
    }

    const ObjectID id{database.next_id_++};
    database.objects_.emplace(id, object);
    return id;
}

void ObjectDB::remove(const ObjectID id, const Object* expected) noexcept {
    ObjectDB& database = instance();
    std::lock_guard lock(database.mutex_);

    const auto found = database.objects_.find(id);
    if (found != database.objects_.end() && found->second == expected) {
        database.objects_.erase(found);
    }
}

Object* ObjectDB::get_instance(const ObjectID id) noexcept {
    if (!id.is_valid()) {
        return nullptr;
    }

    ObjectDB& database = instance();
    std::lock_guard lock(database.mutex_);

    const auto found = database.objects_.find(id);
    return found == database.objects_.end() ? nullptr : found->second;
}

bool ObjectDB::instance_exists(const ObjectID id) noexcept {
    return get_instance(id) != nullptr;
}

} // namespace nle
