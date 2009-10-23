// Copyright (c) 2007-2009 Paul Hodge. All rights reserved.

#include "circa.h"

namespace circa {
namespace less_than_eq_function {

    void evaluate_i(Term* caller)
    {
        as_bool(caller) = int_input(caller,0) <= int_input(caller,1);
    }

    void evaluate_f(Term* caller)
    {
        as_bool(caller) = float_input(caller,0) <= float_input(caller,1);
    }

    void setup(Branch& kernel)
    {
        Term* main = create_overloaded_function(kernel, "less_than_eq");
        import_function_overload(main, evaluate_f, "less_than_eq(int,int) :: bool");
        import_function_overload(main, evaluate_f, "less_than_eq(number,number) :: bool");
    }
}
}
