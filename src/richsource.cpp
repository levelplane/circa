// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "branch.h"
#include "builtins.h"
#include "function.h"
#include "introspection.h"
#include "list_t.h"
#include "parser_util.h"
#include "richsource.h"
#include "source_repro.h"
#include "tagged_value_accessors.h"
#include "term.h"
#include "type.h"

namespace circa {

void append_term_source(RichSource*, Term*);
void append_term_source_default_formatting(RichSource* source, Term* term);

std::string
RichSource::toString()
{
    return _phrases.toString();
}

void append_branch_source(RichSource* source, Branch& branch)
{
    bool newlineNeeded = false;

    for (int i=0; i < branch.length(); i++) {
        Term* term = branch[i];

        if (!should_print_term_source_line(term))
            continue;

        if (newlineNeeded) {
            append_phrase(source, "\n", NULL, token::NEWLINE);
            newlineNeeded = false;
        }

        append_term_source(source, term);

        if (term->hasProperty("syntax:lineEnding"))
            append_phrase(source, term->stringProp("syntax:lineEnding").c_str(),
                term, phrase_type::UNDEFINED);
        else
            newlineNeeded = true;
    }
}

void append_term_source(RichSource* source, Term* term)
{
    assert(term != NULL);

    // Pre whitespace
    append_phrase(source, term->stringPropOptional("syntax:preWhitespace", "").c_str(),
            term, token::WHITESPACE);

    // If the function has a formatSource function, use that.
    if (function_t::get_attrs(term->function).formatSource != NULL) {
        function_t::get_attrs(term->function).formatSource(source, term);

    // Or, check if this is a value term.
    } else if (is_value(term)) {
        // for certain types, don't write "name =" in front
        if (term->type != FUNCTION_TYPE && term->type != TYPE_TYPE)
            append_leading_name_binding(source, term);

        // Special constructor syntax
        if (term->boolPropOptional("constructor", false)) {
            append_phrase(source, (term->type->name + "()").c_str(), term, phrase_type::UNDEFINED);

        // Otherwise use formatSource on type
        } else {
            if (as_type(term->type).formatSource == NULL) {
                std::stringstream out;
                out << "Type " << term->type->name <<
                    " doesn't have a formatSource function";
                throw std::runtime_error(out.str());
            }

            as_type(term->type).formatSource(source, term);
        }
    // Last option; a function call with default formatting.
    } else {
        append_term_source_default_formatting(source, term);
    }

    // Post whitespace
    append_phrase(source, term->stringPropOptional("syntax:postWhitespace", "").c_str(),
            term, token::WHITESPACE);
}

void append_leading_name_binding(RichSource* source, Term* term)
{
    if (term->name == "#out")
        append_phrase(source, "return ", term, phrase_type::UNDEFINED);
    else if (term->name == "")
        return;
    else if (term->boolPropOptional("syntax:implicitNameBinding", false))
        return;
    else if (term->hasProperty("syntax:rebindOperator"))
        return;
    else {
        append_phrase(source, term->name.c_str(), term, phrase_type::UNDEFINED);
        append_phrase(source, term->stringPropOptional("syntax:preEqualsSpace", " ").c_str(),
                term, token::WHITESPACE);
        append_phrase(source, "=", term, token::EQUALS);
        append_phrase(source, term->stringPropOptional("syntax:postEqualsSpace", " ").c_str(),
                term, token::WHITESPACE);
    }
}

void append_source_for_input(RichSource* source, Term* term, int inputIndex)
{
    //TODO
}

void append_term_source_default_formatting(RichSource* source, Term* term)
{
    std::string declarationStyle = term->stringPropOptional("syntax:declarationStyle",
            "function-call");
    bool infix = declarationStyle == "infix";
    std::string functionName = term->stringProp("syntax:functionName");

    // Check for an infix operator with implicit rebinding (like +=).
    if (infix && is_infix_operator_rebinding(functionName)) {
        append_phrase(source, term->name.c_str(), term, phrase_type::UNDEFINED);
        append_phrase(source, " ", term, token::WHITESPACE);
        append_phrase(source, functionName.c_str(), term, phrase_type::INFIX_OPERATOR);
        append_source_for_input(source, term, 1);
        return;
    }

    append_leading_name_binding(source, term);

    // possibly add parens
    int numParens = term->intPropOptional("syntax:parens", 0);
    for (int p=0; p < numParens; p++)
        append_phrase(source, "{", term, token::LPAREN);

    if (declarationStyle == "function-call") {
        append_phrase(source, functionName.c_str(), term, phrase_type::FUNCTION_NAME);

        if (!term->boolPropOptional("syntax:no-parens", false))
            append_phrase(source, "(", term, token::LPAREN);

        for (int i=get_first_visible_input_index(term); i < term->numInputs(); i++)
            append_source_for_input(source, term, i);

        if (!term->boolPropOptional("syntax:no-parens", false))
            append_phrase(source, ")", term, token::RPAREN);
    } else if (declarationStyle == "member-function-call") {

        append_source_for_input(source, term, 0);
        append_phrase(source, ".", term, phrase_type::UNDEFINED);

        append_phrase(source, functionName.c_str(), term, phrase_type::FUNCTION_NAME);

        if (!term->boolPropOptional("syntax:no-parens", false))
            append_phrase(source, "(", term, token::LPAREN);

        for (int i=1; i < term->numInputs(); i++)
            append_source_for_input(source, term, i);

        if (!term->boolPropOptional("syntax:no-parens", false))
            append_phrase(source, ")", term, token::RPAREN);
    } else if (declarationStyle == "dot-concat") {
        append_source_for_input(source, term, 0);
        append_phrase(source, ".", term, phrase_type::UNDEFINED);
        append_phrase(source, functionName.c_str(), term, phrase_type::FUNCTION_NAME);
    } else if (declarationStyle == "infix") {
        append_source_for_input(source, term, 0);
        append_phrase(source, functionName.c_str(), term, phrase_type::INFIX_OPERATOR);
        append_source_for_input(source, term, 1);
    } else if (declarationStyle == "arrow-concat") {
        append_source_for_input(source, term, 0);
        append_phrase(source, "->", term, phrase_type::UNDEFINED);
        append_source_for_input(source, term, 1);
        append_phrase(source, functionName.c_str(), term, phrase_type::INFIX_OPERATOR);
    }
}

void append_phrase(RichSource* source, const char* str, Term* term, int type)
{
    // No-op if string is empty
    if (str[0] == 0)
        return;

    TaggedValue* list = make_list(source->_phrases.append());
    make_string(list_t::append(list), str);
    make_ref(list_t::append(list), term);
    make_int(list_t::append(list), type);
}

std::string unformat_rich_source(RichSource* source)
{
    std::stringstream strm;

    for (int i=0; i < source->_phrases.numElements(); i++) {
        TaggedValue* phrase = source->_phrases[i];
        strm << as_string((*phrase)[0]);
    }
    return strm.str();
}

} // namespace circa