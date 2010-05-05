// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "circa.h"

namespace circa {
namespace greater_than_function {

    void evaluate_i(EvalContext*, Term* caller)
    {
        set_bool(caller, int_input(caller,0) > int_input(caller,1));
    }

    void evaluate_f(EvalContext*, Term* caller)
    {
        set_bool(caller, float_input(caller,0) > float_input(caller,1));
    }

    void setup(Branch& kernel)
    {
        Term* gt_i = import_function(kernel, evaluate_i, "greater_than_i(int,int) -> bool");
        Term* gt_f = import_function(kernel, evaluate_f, "greater_than_f(number,number) -> bool");

        create_overloaded_function(kernel, "greater_than", RefList(gt_i, gt_f));
    }
}
}
