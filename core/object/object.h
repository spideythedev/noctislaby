#pragma once

#include "object_id.h"

namespace nle {

class ObjectDB;

class Object {
public:
    Object();
    virtual ~Object();

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&&) = delete;
    Object& operator=(Object&&) = delete;

    [[nodiscard]] ObjectID get_instance_id() const noexcept;

    [[nodiscard]] virtual const char* get_class_name() const noexcept;

private:
    ObjectID instance_id_;

    friend class ObjectDB;
};

} // namespace nle
