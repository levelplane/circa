// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "circa.h"
#include "importing_macros.h"

namespace circa {
namespace refactor_function {

    CA_START_FUNCTIONS;

    CA_DEFINE_FUNCTION(rename, "rename(Ref r, string s)")
    {
        rename(as_ref(INPUT(0)), as_string(INPUT(1)));
    }

    void setup(Branch& kernel)
    {
        Branch& ns = create_namespace(kernel, "refactor");
        CA_SETUP_FUNCTIONS(ns)
    }
}
} // namespace circa
