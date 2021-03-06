// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "common_headers.h"

#include "circa_internal.h"
#include "filesystem_dummy.h"

namespace circa {
namespace stateful_code_tests {

void test_is_function_stateful()
{
    Branch branch;

    // Explicit state argument
    Term* f = branch.compile("def f(state any s) {}");
    test_assert(&branch);
    test_assert(is_function_stateful(f));

    Term* x = branch.compile("def x() {}");
    test_assert(!is_function_stateful(x));
    
    // Implicit state
    Term* g = branch.compile("def g() { f() }");
    test_assert(is_function_stateful(g));

    Term* y = branch.compile("def y() {}");
    test_assert(!is_function_stateful(y));
}

CA_FUNCTION(simple_func_with_state_arg)
{
    caValue* in = INPUT(0);
    int i = 0;
    if (is_int(in))
        i = as_int(in);
    set_int(EXTRA_OUTPUT(0), i + 1);
}

void test_simple_func_with_state_arg()
{
    Branch branch;
    import_function(&branch, simple_func_with_state_arg, "f(state int i :output)");

    // Test some static checks
    Function* func = as_function(branch["f"]);

    Term* stateOutput = function_get_output_placeholder(func, 1);
    test_assert(function_is_state_input(stateOutput));

    // Test with a normal call, explicit argument.
    branch.compile("a = 3");
    branch.compile("f(state = a)");
    Stack context;
    evaluate_save_locals(&context, &branch);
    test_equals(branch["a"], "4");

    // Test with an implicit argument
    branch.compile("f()");

    evaluate_save_locals(&context, &branch);
    test_equals(&context.state, "{_f_1: 1}");

    evaluate_save_locals(&context, &branch);
    test_equals(&context.state, "{_f_1: 2}");
}

void test_get_type_from_branches_stateful_terms()
{
    Branch branch;
    branch.eval("a = 0");
    branch.eval("state number b");
    branch.eval("c = 'hello'");
    branch.eval("state bool d");

    Branch type;
    
    get_type_from_branches_stateful_terms(&branch, &type);

    test_assert(type.length() == 2);
    test_assert(is_value(type[0]));
    test_equals(type[0]->type->name, "number");
    test_assert(is_value(type[0]));
    test_equals(type[1]->type->name, "bool");
}

void initial_value()
{
    Branch branch;
    Stack context;

    Term* i = branch.compile("state i = 3");
    Term* j = branch.compile("state int j = 4");
    evaluate_save_locals(&context, &branch);

    test_equals(i, "3");
    test_equals(j, "4");
}

void initialize_from_expression()
{
    Branch branch;
    branch.compile("a = 1 + 2");
    branch.compile("b = a * 2");
    Term *c = branch.compile("state c = b");

    test_assert(&branch);

    evaluate_branch(&branch);

    test_equals(to_float(c), 6);

    branch.clear();
    Term* d = branch.compile("d = 5");
    Term* e = branch.compile("state e = d");
    Stack context;
    evaluate_save_locals(&context, &branch);
    test_equals(e, "5");

    set_int(d, 10);
    evaluate_branch(&context, &branch);
    test_equals(e->asInt(), 5);
}

int NEXT_UNIQUE_OUTPUT = 0;

CA_FUNCTION(_unique_output)
{
    set_int(OUTPUT, NEXT_UNIQUE_OUTPUT++);
}

List SPY_RESULTS;

CA_FUNCTION(_spy)
{
    copy(INPUT(0), SPY_RESULTS.append());
}

void one_time_assignment_inside_for_loop()
{
    Branch branch;

    import_function(&branch, _unique_output, "unique_output() -> int");
    import_function(&branch, _spy, "spy(int)");
    branch.compile("for i in [1 1 1] { state s = unique_output(); spy(s) }");
    test_assert(&branch);

    NEXT_UNIQUE_OUTPUT = 0;
    SPY_RESULTS.clear();

    Stack context;
    evaluate_branch(&context, &branch);

    test_equals(&SPY_RESULTS, "[0, 1, 2]");

    SPY_RESULTS.clear();
    evaluate_branch(&context, &branch);

    test_equals(&SPY_RESULTS, "[0, 1, 2]");
}

void explicit_state()
{
    Branch branch;
    branch.compile("state s");
    branch.compile("s = 1");

    Stack context;
    evaluate_branch(&context, &branch);

    test_equals(context.state.toString(), "{s: 1}");
}

void implicit_state()
{
    Branch branch;
    branch.compile("def f() { state s; s = 1 }");
    branch.compile("f()");

    Stack context;
    evaluate_branch(&context, &branch);
    
    test_equals(context.state.toString(), "{_f: {s: 1}}");
}

void bug_with_top_level_state()
{
    // This code once caused an assertion failure
    Branch branch;
    branch.compile("state s = 1");
    branch.compile("def f() { state t }");
    branch.compile("f()");
    evaluate_branch(&branch);
}

void bug_with_state_and_plus_equals()
{
    Branch branch;
    branch.compile("state int count = 0; count += 1");

    Stack context;
    evaluate_branch(&context, &branch);
}

void subroutine_unique_name_usage()
{
    Branch branch;
    branch.compile("def f() { state s = 0; s += 1; s += 2; s += 5 } f()");
    Stack context;
    evaluate_branch(&context, &branch);

    test_equals(&context.state, "{_f: {s: 8}}");
}

void subroutine_early_return()
{
    Branch branch;
    branch.compile("def f()->int { state s = 2; return 0; s = 4; } f()");
    Stack context;
    evaluate_branch(&context, &branch);
    test_equals(&context.state, "{_f: {s: 2}}");
}

void test_state_var_needs_cast()
{
    Branch branch;

    internal_debug_function::spy_clear();

    branch.compile("state Point blah");
    branch.compile("test_spy(blah.x)");
    branch.compile("blah = [blah.x + 1, 0]");

    Stack context;

    evaluate_branch(&context, &branch);
    test_assert(!context.errorOccurred);

    evaluate_branch(&context, &branch);
    test_assert(!context.errorOccurred);

    test_equals(internal_debug_function::spy_results(), "[0.0, 1.0]");
}

void test_state_var_default_needs_cast()
{
    Branch branch;
    branch.compile("state Point p = [3 4]");
    branch.compile("test_spy(p.x)");

    internal_debug_function::spy_clear();

    Stack context;
    evaluate_branch(&context, &branch);
    test_assert(!context.errorOccurred);
    test_equals(internal_debug_function::spy_results(), "[3.0]");

    branch.clear();
    reset(&context.state);
    branch.compile("state Point p = [1 2 3]");
    evaluate_branch(&context, &branch);
    test_assert(context.errorOccurred);
}

std::string code_to_state_shape(const char* code)
{
    Branch branch;
    branch.compile(code);
    caValue description;
    describe_state_shape(&branch, &description);
    return description.toString();
}

void test_describe_state_shape()
{
    test_equals(code_to_state_shape("a = 1; for i in [2 3] {}; if true {}"), "null");
    test_equals(code_to_state_shape("state a = 1"), "{a: 'int'}");
    test_equals(code_to_state_shape("if true { state a = 1 }"),
            "{_if_block: [{a: 'int'}, null]}");
    test_equals(code_to_state_shape("if true { state a = 1 } else {state b}"),
            "{_if_block: [{a: 'int'}, {b: 'any'}]}");
    test_equals(code_to_state_shape("for i in [1 2 3] { 1 }"), "null");
    test_equals(code_to_state_shape("for i in [1 2 3] { state s }"),
            "{_for: [{s: 'any'}, :Repeat]}");
    test_equals(code_to_state_shape(
        "for i in [1 2 3] { for j in [1 2 3] { state s } }"),
            "{_for: [{_for: [{s: 'any'}, :Repeat]}, :Repeat]}");
}

void test_strip_abandoned_state()
{
    Branch branch;

    Term* s = branch.compile("state s = 1");
    Term* t = branch.compile("state t = 2");
    Term* x = branch.compile("state x = 'hi'");

    Stack context;
    evaluate_branch(&context, &branch);

    caValue trash;

    test_equals(&context.state, "{s: 1, t: 2, x: 'hi'}");

    remove_term(t);
    strip_orphaned_state(&branch, &context.state, &trash);
    test_equals(&context.state, "{s: 1, x: 'hi'}");
    test_equals(&trash, "{t: 2}");

    remove_term(x);
    strip_orphaned_state(&branch, &context.state, &trash);
    test_equals(&context.state, "{s: 1}");
    test_equals(&trash, "{x: 'hi'}");

    remove_term(s);
    strip_orphaned_state(&branch, &context.state, &trash);
    test_equals(&context.state, "null");
    test_equals(&trash, "{s: 1}");
}

void test_preserve_state_in_nested_include_file()
{
    // File 'a' includes 'b', both have state. When changing 'a', make sure that
    // the state in 'b' is preserved.
    Branch branch;
    FakeFileSystem files;

    internal_debug_function::oracle_clear();
    internal_debug_function::oracle_send(1);
    internal_debug_function::oracle_send(2);
    internal_debug_function::oracle_send(3);
    internal_debug_function::oracle_send(3);
    
    // (send a double 3, this test doesn't care if the test_oracle call gets reevaluated after
    // the script change).

    files.set("a", "include('b'); state s = test_oracle()");
    files.set("b", "state x = test_oracle()");
    branch.compile("include('a')");

    caValue desc;
    describe_state_shape(&branch, &desc);
    
    Stack context;
    evaluate_branch(&context, &branch);

    test_equals(&context.state, "{_include: {_include: {x: 1}, s: 2}}");

    files.set("a", "include('b'); state t = test_oracle()");
    evaluate_branch(&context, &branch);

    // TEST_DISABLED: Requires state pruning
    //test_equals(&context.state, "{_include: {_include: {x: 1}, t: 3}}");
}

void test_that_initial_value_doesnt_get_reevaluated()
{
    return; // TEST_DISABLED
    Branch branch;

    internal_debug_function::spy_clear();

    branch.compile("def f()->int { test_spy('call'); return 1 }");
    branch.compile("state s = f()");

    test_equals(internal_debug_function::spy_results(), "[]");

    Stack context;
    evaluate_branch(&context, &branch);
    evaluate_branch(&context, &branch);
    evaluate_branch(&context, &branch);

    test_equals(internal_debug_function::spy_results(), "['call']");

    internal_debug_function::spy_clear();
    reset(&context.state);
    evaluate_branch(&context, &branch);
    reset(&context.state);
    evaluate_branch(&context, &branch);
    reset(&context.state);
    evaluate_branch(&context, &branch);

    test_equals(internal_debug_function::spy_results(), "['call', 'call', 'call']");
}

void register_tests()
{
    REGISTER_TEST_CASE(stateful_code_tests::test_is_function_stateful);
    REGISTER_TEST_CASE(stateful_code_tests::test_simple_func_with_state_arg);
    REGISTER_TEST_CASE(stateful_code_tests::test_get_type_from_branches_stateful_terms);
    REGISTER_TEST_CASE(stateful_code_tests::initial_value);
    REGISTER_TEST_CASE(stateful_code_tests::initialize_from_expression);
    REGISTER_TEST_CASE(stateful_code_tests::one_time_assignment_inside_for_loop);
    REGISTER_TEST_CASE(stateful_code_tests::explicit_state);
    REGISTER_TEST_CASE(stateful_code_tests::implicit_state);
    REGISTER_TEST_CASE(stateful_code_tests::bug_with_top_level_state);
    REGISTER_TEST_CASE(stateful_code_tests::bug_with_state_and_plus_equals);
    REGISTER_TEST_CASE(stateful_code_tests::subroutine_unique_name_usage);
    REGISTER_TEST_CASE(stateful_code_tests::subroutine_early_return);
    REGISTER_TEST_CASE(stateful_code_tests::test_state_var_needs_cast);
    REGISTER_TEST_CASE(stateful_code_tests::test_state_var_default_needs_cast);
    REGISTER_TEST_CASE(stateful_code_tests::test_describe_state_shape);
    REGISTER_TEST_CASE(stateful_code_tests::test_strip_abandoned_state);
    REGISTER_TEST_CASE(stateful_code_tests::test_preserve_state_in_nested_include_file);
    REGISTER_TEST_CASE(stateful_code_tests::test_that_initial_value_doesnt_get_reevaluated);
}

} // namespace stateful_code_tests

} // namespace circa
