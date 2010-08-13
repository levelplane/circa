// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "common_headers.h"

#include "circa.h"
#include "types/dict.h"

namespace circa {
namespace inline_state_function {

    CA_START_FUNCTIONS;

    CA_DEFINE_FUNCTION(get_top_level_state, "get_top_level_state() -> any")
    {
        copy(&CONTEXT->topLevelState, OUTPUT);
    }
    CA_DEFINE_FUNCTION(set_top_level_state, "set_top_level_state() -> any")
    {
        copy(INPUT(0), &CONTEXT->topLevelState);
    }

    CA_DEFINE_FUNCTION(get_state_field, "get_state_field(any, string name) -> any")
    {
        TaggedValue *container = INPUT(0);
        if (!is_dict(container)) make_dict(container);

        Dict* dict = Dict::checkCast(container);
        TaggedValue* value = dict->get(STRING_INPUT(1));
        if (value) {
            // todo: check if we need to cast this value
            copy(value, OUTPUT);
        } else {
            change_type(OUTPUT, type_contents(CALLER->type));
        }
    }

    CA_DEFINE_FUNCTION(set_state_field,
            "set_state_field(any container, string name, any field) -> any")
    {
        copy(INPUT(0), OUTPUT);
        TaggedValue *container = OUTPUT;
        touch(container);
        if (!is_dict(container)) make_dict(container);
        Dict* dict = Dict::checkCast(container);
        dict->set(STRING_INPUT(1), INPUT(2));
    }

#ifndef BYTECODE
    CA_FUNCTION(empty_evaluate)
    {
    }
#endif

    void formatSource(StyledSource* source, Term* term)
    {
        append_phrase(source, "state ", term, token::STATE);

        if (term->hasProperty("syntax:explicitType")) {
            append_phrase(source, term->stringProp("syntax:explicitType"),
                    term, phrase_type::TYPE_NAME);
            append_phrase(source, " ", term, token::WHITESPACE);
        }

        append_phrase(source, term->name.c_str(), term, phrase_type::TERM_NAME);

        if (term->hasProperty("initializedBy")) {
            Term* initializedBy = term->refProp("initializedBy");
            append_phrase(source, " = ", term, phrase_type::UNDEFINED);
            if (initializedBy->name != "")
                append_phrase(source, get_relative_name(term, initializedBy),
                        term, phrase_type::TERM_NAME);
            else
                format_term_source(source, initializedBy);
        }
    }

    void setup(Branch& kernel)
    {
#ifdef BYTECODE
        CA_SETUP_FUNCTIONS(kernel);
#else
        STATEFUL_VALUE_FUNC = import_function(kernel, empty_evaluate,
                "stateful_value(any next_val +optional) -> any");
        function_t::get_attrs(STATEFUL_VALUE_FUNC).formatSource = formatSource;
#endif
    }
}
}
