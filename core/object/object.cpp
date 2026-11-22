#include "object.h"

#include "object_db.h"

namespace nle {

Object::Object() : instance_id_(ObjectDB::add(this)) {}

Object::~Object() {
    ObjectDB::remove(instance_id_, this);
}

ObjectID Object::get_instance_id() const noexcept {
    return instance_id_;
}

const char* Object::get_class_name() const noexcept {
    return "Object";
}

} // namespace nle
