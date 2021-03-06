// Copyright (c) 2007-2010 Andrew Fischer. All rights reserved

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace instance_function {
    
    CA_START_FUNCTIONS;

    CA_DEFINE_FUNCTION(instance, "instance(Type t) -> any")
    {
        create(unbox_type(INPUT(0)), OUTPUT);
    }

    Type* specializeType(Term* caller)
    {
        return as_type(caller->input(0));
    }

    void setup(Branch* kernel)
    {
        CA_SETUP_FUNCTIONS(kernel);
        INSTANCE_FUNC = kernel->get("instance");
        as_function(INSTANCE_FUNC)->specializeType = specializeType;
    }
}
} // namespace circa
