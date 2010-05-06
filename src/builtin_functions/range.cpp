// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "circa.h"

namespace circa {
namespace range_function {

    void evaluate(EvalContext*, Term* caller)
    {
        int start = int_input(caller, 0);
        int max = int_input(caller, 1);
        
        Branch& branch = as_branch(caller);

        int count = abs(max-start);
        resize_list(branch, count, INT_TYPE);

        int increment = start < max ? 1 : -1;
        
        int val = start;
        for (int i=0; i < count; i++) {
            set_int(branch[i], val);
            val += increment;
        }
    }

    void setup(Branch& kernel)
    {
        RANGE_FUNC = import_function(kernel, evaluate,
                "range(int start, int max) -> List; 'Return a list of integers from start to max-1' end");
    }
}
} // namespace circa
