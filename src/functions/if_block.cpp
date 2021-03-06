// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace if_block_function {

    void formatSource(StyledSource* source, Term* term)
    {
        format_name_binding(source, term);

        Branch* contents = nested_contents(term);

        int index = 0;
        while (contents->get(index)->function == FUNCS.input)
            index++;

        bool firstCase = true;

        for (; index < contents->length(); index++) {
            Term* caseTerm = contents->get(index);

            if (caseTerm->function != FUNCS.case_func)
                break;

            if (is_hidden(caseTerm))
                continue;

            append_phrase(source,
                    caseTerm->stringPropOptional("syntax:preWhitespace", ""),
                    caseTerm, TK_WHITESPACE);

            if (firstCase) {
                append_phrase(source, "if ", caseTerm, phrase_type::KEYWORD);
                format_source_for_input(source, caseTerm, 0);
                firstCase = false;
            } else if (caseTerm->input(0) != NULL) {
                append_phrase(source, "elif ", caseTerm, phrase_type::KEYWORD);
                format_source_for_input(source, caseTerm, 0);
            }
            else
                append_phrase(source, "else", caseTerm, phrase_type::UNDEFINED);

            // whitespace following the if/elif/else
            append_phrase(source,
                    caseTerm->stringPropOptional("syntax:lineEnding", ""),
                    caseTerm, TK_WHITESPACE);

            format_branch_source(source, nested_contents(caseTerm), caseTerm);
        }
    }

    void setup(Branch* kernel)
    {
        FUNCS.if_block = import_function(kernel, NULL, "if_block() -> any");
        as_function(FUNCS.if_block)->formatSource = formatSource;

        FUNCS.case_func = import_function(kernel, NULL, "case(bool :optional)");
    }
}
}
