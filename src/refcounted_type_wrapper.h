// Copyright (c) Paul Hodge. See LICENSE file for license terms.

#include "tagged_value.h"

#pragma once

namespace circa {
namespace intrusive_refcounted {

    template <typename T>
    void initialize(Type* type, TaggedValue* value)
    {
        T* instance = new T();
        instance->_refCount = 1;
        value->value_data.ptr = instance;
    }

    template <typename T>
    void release(Type*, TaggedValue* value)
    {
        T* instance = (T*) value->value_data.ptr;

        ca_assert(instance != NULL);

        instance->_refCount--;
        if (instance->_refCount <= 0)
            delete instance;
        value->value_data.ptr = NULL;
    }

    template <typename T>
    void set(TaggedValue* tv, Type* type, T* value)
    {
        // Increase refcount first, in case the value already has this object.
        value->_refCount++;

        change_type_no_initialize(tv, type);
        tv->value_data = value;
    }

    template <typename T>
    void copy(Type* type, TaggedValue* source, TaggedValue* dest)
    {
        // Increase refcount first, in case 'dest' already has this object.
        T* instance = (T*) source->value_data.ptr;
        ca_assert(instance->_refCount > 0);
        instance->_refCount++;

        change_type_no_initialize(dest, type);
        dest->value_data.ptr = instance;
    }

    template <typename T>
    void setup_type(Type* type)
    {
        type->initialize = initialize<T>;
        type->release = release<T>;
        type->copy = copy<T>;
    }
}


} // namespace circa
