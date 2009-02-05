// Copyright 2008 Paul Hodge

#include "common_headers.h"

#include "circa.h"

namespace circa {
namespace newparser_tests {

void test_comment()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "-- this is a comment");

    test_assert(branch[0]->function == COMMENT_FUNC);
    test_equals(branch[0]->state->field(0)->asString(), " this is a comment");
    test_assert(branch.numTerms() == 1);

    newparser::compile(branch, newparser::statement, "--");
    test_assert(branch.numTerms() == 2);
    test_assert(branch[1]->function == COMMENT_FUNC);
    test_equals(branch[1]->state->field(0)->asString(), "");
}

void test_blank_line()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "");
    test_assert(branch.numTerms() == 1);
    test_assert(branch[0]->function == COMMENT_FUNC);
    test_equals(branch[0]->state->field(0)->asString(), "");
}

void test_literal_integer()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "1");
    test_assert(branch.numTerms() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asInt() == 1);
}

void test_literal_float()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "1.0");
    test_assert(branch.numTerms() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asFloat() == 1.0);
}

void test_literal_string()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "\"hello\"");
    test_assert(branch.numTerms() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asString() == "hello");
}

void test_name_binding()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "a = 1");
    test_assert(branch.numTerms() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asInt() == 1);
    test_assert(branch[0] == branch["a"]);
    test_assert(branch[0]->name == "a");
}

void test_function_call()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "add(1.0,2.0)");
    test_assert(branch.numTerms() == 3);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asFloat() == 1.0);
    test_assert(is_value(branch[1]));
    test_assert(branch[1]->asFloat() == 2.0);

    test_assert(branch[2]->function == ADD_FUNC);
    test_assert(branch[2]->input(0) == branch[0]);
    test_assert(branch[2]->input(1) == branch[1]);
}

void test_identifier()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "a = 1.0");
    test_assert(branch.numTerms() == 1);

    Term* a = newparser::compile(branch, newparser::statement, "a");

    test_assert(branch.numTerms() == 1);
    test_assert(a == branch[0]);

    newparser::compile(branch, newparser::statement, "add(a,a)");
    test_assert(branch.numTerms() == 2);
    test_assert(branch[1]->input(0) == a);
    test_assert(branch[1]->input(1) == a);
}

void test_rebind()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "a = 1.0");
    newparser::compile(branch, newparser::statement, "add(@a,a)");

    test_assert(branch.numTerms() == 2);
    test_assert(branch["a"] == branch[1]);
}

void test_infix()
{
    Branch branch;
    newparser::compile(branch, newparser::statement, "1.0 + 2.0");

    test_assert(branch.numTerms() == 3);
    test_assert(branch[0]->asFloat() == 1.0);
    test_assert(branch[1]->asFloat() == 2.0);
    test_assert(branch[2]->function == ADD_FUNC);
    test_assert(branch[2]->input(0) == branch[0]);
    test_assert(branch[2]->input(1) == branch[1]);

    branch.clear();
}

void test_type_decl()
{
    Branch branch;
    Term* typeTerm = newparser::compile(branch, newparser::statement,
            "type Mytype {\nint a\nfloat b\n}");
    Type& type = as_type(typeTerm);

    test_equals(type.name, "Mytype");
    test_equals(typeTerm->name, "Mytype");

    test_assert(type.fields[0].type == INT_TYPE);
    test_equals(type.fields[0].name, "a");
    test_assert(type.fields[1].type == FLOAT_TYPE);
    test_equals(type.fields[1].name, "b");
}

void test_function_decl()
{
    Branch branch;
    Term* funcTerm = newparser::compile(branch, newparser::statement,
            "function Myfunc(string what, string hey, int yo) -> bool\n"
            "  whathey = concat(what,hey)\n"
            "  return yo > 3\n"
            "end");

    Function& func = as_function(funcTerm);

    test_equals(funcTerm->name, "Myfunc");
    test_equals(func.name, "Myfunc");

    test_assert(func.inputTypes[0] == STRING_TYPE);
    test_assert(func.getInputProperties(0).name == "what");
    test_assert(func.inputTypes[1] == STRING_TYPE);
    test_assert(func.getInputProperties(1).name == "hey");
    test_assert(func.inputTypes[2] == INT_TYPE);
    test_assert(func.getInputProperties(2).name == "yo");
    test_assert(func.outputType == BOOL_TYPE);

    Branch& funcbranch = func.subroutineBranch;

    print_raw_branch(funcbranch, std::cout);
    test_assert(funcbranch.numTerms() == 6);
    test_assert(funcbranch[0]->name == "what");
    test_assert(funcbranch[1]->name == "hey");
    test_assert(funcbranch[2]->name == "yo");
    test_assert(funcbranch[3]->name == "whathey");
}

void register_tests()
{
    REGISTER_TEST_CASE(newparser_tests::test_comment);
    REGISTER_TEST_CASE(newparser_tests::test_blank_line);
    REGISTER_TEST_CASE(newparser_tests::test_literal_integer);
    REGISTER_TEST_CASE(newparser_tests::test_literal_float);
    REGISTER_TEST_CASE(newparser_tests::test_literal_string);
    REGISTER_TEST_CASE(newparser_tests::test_name_binding);
    REGISTER_TEST_CASE(newparser_tests::test_function_call);
    REGISTER_TEST_CASE(newparser_tests::test_identifier);
    REGISTER_TEST_CASE(newparser_tests::test_rebind);
    REGISTER_TEST_CASE(newparser_tests::test_infix);
    REGISTER_TEST_CASE(newparser_tests::test_type_decl);
    //REGISTER_TEST_CASE(newparser_tests::test_function_decl);
}

} // namespace newparser_tests
} // namespace circa
