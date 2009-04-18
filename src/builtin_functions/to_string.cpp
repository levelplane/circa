// Copyright 2008 Paul Hodge

#include "circa.h"

namespace circa {
namespace to_string_function {

    void evaluate(Term* caller)
    {
        Term* term = caller->input(0);
        
        Type::ToStringFunc func = as_type(term->type).toString;
        
        if (func == NULL) {
            as_string(caller) = std::string("<" + as_type(term->type).name
                    + " " + term->name + ">");
        } else {
            as_string(caller) = func(term);
        }
    }

    void setup(Branch& kernel)
    {
        Term* main_func = import_function(kernel, evaluate, "to_string(any) -> string");
        as_function(main_func).pureFunction = true;
    }
}
} // namespace circa