// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace comment_function {

    void formatSource(StyledSource* source, Term* term)
    {
        append_phrase(source, term->stringProp("comment"), term, TK_COMMENT);
    }

    void setup(Branch* kernel)
    {
        FUNCS.comment = import_function(kernel, NULL, "comment()");
        as_function(FUNCS.comment)->formatSource = formatSource;
        function_set_empty_evaluation(as_function(FUNCS.comment));
    }
}
}
