// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

// Unit tests for parser.cpp

#include "common_headers.h"

#include "circa_internal.h"

namespace circa {
namespace parser_tests {

void test_comment()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "-- this is a comment");

    test_assert(branch[0]->function == FUNCS.comment);
    test_equals(branch[0]->stringProp("comment"), "-- this is a comment");
    test_assert(branch.length() == 1);

    parser::compile(&branch, parser::statement, "--");
    test_assert(branch.length() == 2);
    test_assert(branch[1]->function == FUNCS.comment);
    test_equals(branch[1]->stringProp("comment"), "--");
}

void test_blank_line()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "\n");
    test_assert(branch.length() == 1);
    test_assert(branch[0]->function == FUNCS.comment);
    test_equals(branch[0]->stringProp("comment"), "");
}

void test_literal_integer()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "1");
    test_assert(branch.length() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asInt() == 1);
}

void test_literal_float()
{
    Branch branch;
    Term* a = parser::compile(&branch, parser::statement, "1.0");
    test_assert(branch.length() == 1);
    test_assert(branch[0] == a);
    test_assert(is_value(a));
    test_assert(a->asFloat() == 1.0);
    test_equals(get_step(a), .1f);

    Term* b = parser::compile(&branch, parser::statement_list, "5.200");
    test_equals(b->type->name, "number");
    test_equals(get_term_source_text(b), "5.200");
    test_equals(get_step(b), .001f);
}

void test_literal_string()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "\"hello\"");
    test_assert(branch.length() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asString() == "hello");
}

void test_name_binding()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "a = 1");
    test_assert(branch.length() == 1);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asInt() == 1);
    test_assert(branch[0] == branch["a"]);
    test_assert(branch[0]->name == "a");
}

void test_function_call()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "add_f(1.0,2.0)");
    test_assert(branch.length() == 3);
    test_assert(is_value(branch[0]));
    test_assert(branch[0]->asFloat() == 1.0);
    test_assert(is_value(branch[1]));
    test_assert(branch[1]->asFloat() == 2.0);

    test_assert(branch[2]->function->name == "add_f");
    test_assert(branch[2]->input(0) == branch[0]);
    test_assert(branch[2]->input(1) == branch[1]);
}

void test_identifier()
{
    Branch branch;
    Term* a = parser::compile(&branch, parser::statement, "a = 1.0");
    test_assert(branch.length() == 1);

    test_assert(branch.length() == 1);
    test_assert(a == branch[0]);

    parser::compile(&branch, parser::statement, "add(a,a)");
    test_assert(branch.length() == 2);
    test_assert(branch[1]->input(0) == a);
    test_assert(branch[1]->input(1) == a);
}

void test_rebind()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "a = 1.0");
    parser::compile(&branch, parser::statement, "add(@a,a)");

    test_assert(branch.length() == 2);
    test_assert(branch["a"] == branch[1]);
}

void test_infix()
{
    Branch branch;
    parser::compile(&branch, parser::statement, "1.0 + 2.0");

    test_assert(branch.length() == 3);
    test_assert(branch[0]->asFloat() == 1.0);
    test_assert(branch[1]->asFloat() == 2.0);
    test_assert(branch[2]->function->name == "add");
    test_assert(branch[2]->input(0) == branch[0]);
    test_assert(branch[2]->input(1) == branch[1]);

    branch.clear();
}

void test_type_decl()
{
    Branch branch;
    Term* typeTerm = parser::compile(&branch, parser::statement,
            "type Mytype {\nint a\nnumber b\n}");

    test_equals(as_type(typeTerm)->name, "Mytype");
    test_equals(typeTerm->name, "Mytype");

    Branch* contents = nested_contents(typeTerm);

    test_equals(contents->get(0)->type->name, "int");
    test_equals(contents->get(0)->name, "a");
    test_equals(contents->get(1)->type->name, "number");
    test_equals(contents->get(1)->name, "b");
}

void test_function_decl()
{
    Branch branch;
    Term* func = parser::compile(&branch, parser::statement,
            "def Myfunc(string what, string hey, int yo) -> bool\n"
            "  whathey = concat(what,hey)\n"
            "  return(yo > 3)\n");

    Function* funcAttrs = as_function(func);

    test_equals(func->name, "Myfunc");
    test_equals(funcAttrs->name, "Myfunc");

    test_equals(function_get_input_type(func, 0)->name, "string");
    test_assert(function_get_input_name(funcAttrs, 0) == "what");
    test_equals(function_get_input_type(func, 1)->name, "string");
    test_assert(function_get_input_name(funcAttrs, 1) == "hey");
    test_equals(function_get_input_type(func, 2)->name, "int");
    test_assert(function_get_input_name(funcAttrs, 2) == "yo");
    test_equals(function_get_output_type(func, 0)->name, "bool");

    Branch* funcbranch = function_contents(func);

    int i = 0;
    test_equals(funcbranch->get(i++)->name, "what");
    test_equals(funcbranch->get(i++)->name, "hey");
    test_equals(funcbranch->get(i++)->name, "yo");
    // The parser might insert a comment here for the newline
    if (funcbranch->get(i)->function->name == "comment")
        i++;
    test_equals(funcbranch->get(i)->name, "whathey");
    test_equals(funcbranch->get(i)->function->name, "concat");
    test_assert(funcbranch->get(i)->input(0) == funcbranch->get(0));
    test_assert(funcbranch->get(i)->input(1) == funcbranch->get(1));
    i++;
    int three = i;
    test_assert(funcbranch->get(i++)->asInt() == 3);
    test_equals(funcbranch->get(i)->function->name, "greater_than");
    test_assert(funcbranch->get(i)->input(0) == funcbranch->get(2));
    test_assert(funcbranch->get(i)->input(1) == funcbranch->get(three));
}

void test_stateful_value_decl()
{
    Branch branch;
    Term* a = parser::compile(&branch, parser::statement, "state int a");

    test_assert(is_declared_state(a));
    test_assert(a->name == "a");
    test_equals(a->type->name, "int");
    test_assert(branch["a"] == a);

    Term* b = parser::compile(&branch, parser::statement, "state b = 5.0");
    test_assert(b->name == "b");
    test_assert(is_declared_state(b));

    test_equals(b->type->name, "number");
    test_assert(branch["b"] == b);
    test_assert(!is_float(b) || as_float(b) == 0); // shouldn't have this value yet

    Term* c = parser::compile(&branch, parser::statement, "state number c = 7.5");
    test_assert(c->name == "c");
    test_assert(is_declared_state(c));
    test_equals(c->type->name, "number");
    test_assert(branch["c"] == c);
    test_assert(!is_float(c) || as_float(c) == 0); // shouldn't have this value yet
}

void test_arrow_concatenation()
{
    Branch branch;
    Term* a = parser::compile(&branch, parser::statement, "1 -> to_string");

    test_assert(branch[0]->asInt() == 1);
    test_assert(branch[1] == a);
    test_equals(branch[1]->function->name, "to_string");
    test_assert(branch[1]->input(0) == branch[0]);
    test_equals(branch[1]->type->name, "string");
    test_assert(branch.length() == 2);
}

void test_arrow_concatenation2()
{
    Branch branch;
    Term* a = parser::compile(&branch, parser::statement,
        "0.0 -> cos -> to_string");

    test_assert(branch[0]->asFloat() == 0.0);
    test_equals(branch[1]->function->name, "cos");
    test_assert(branch[1]->input(0) == branch[0]);
    test_equals(branch[2]->function->name, "to_string");
    test_assert(branch[2]->input(0) == branch[1]);
    test_assert(branch[2] == a);
    test_assert(branch.length() == 3);
}

void test_dot_concatenation()
{
    Branch branch;

    branch.compile("s = Set()");

    // This function should rebind 's'
    Term *s = branch.compile("s.add(1)");

    test_assert(branch.length() == 3);
    test_assert(is_value(branch[0]));
    test_assert(is_value(branch[1]));
    test_assert(branch[1]->asInt() == 1);
    test_equals(as_function(branch[2]->function)->name, "Set.add");
    test_assert(branch["s"] == s);
}

void test_syntax_hints()
{
    Branch branch;

    Term* t = parser::compile(&branch, parser::statement, "concat('a', 'b')");
    test_equals(get_input_syntax_hint(t, 0, "preWhitespace"), "");
    test_equals(get_input_syntax_hint(t, 0, "postWhitespace"), ",");
    test_equals(get_input_syntax_hint(t, 1, "preWhitespace"), " ");
    test_equals(get_input_syntax_hint(t, 1, "postWhitespace"), "");
}

void test_implicit_copy_by_identifier()
{
    Branch branch;
    Term* a = branch.compile("a = 1");
    Term* b = branch.compile("b = a");

    test_assert(b->function == FUNCS.copy);
    test_assert(b->input(0) == a);
}

void test_rebinding_infix_operator()
{
    Branch branch;
    branch.compile("i = 1.0");
    Term* i = branch.compile("i += 1.0");

    test_assert(branch["i"] == i);
    test_assert(i->function->name == "add");
    test_assert(i->name == "i");
    test_assert(i->input(0)->name == "i");
}

void test_infix_whitespace()
{
    Branch branch;
    branch.compile("a = 1");
    branch.compile("b = 1");

    Term* term = parser::compile(&branch, parser::infix_expression, "  a + b");
    test_equals(term->stringProp("syntax:preWhitespace"), "  ");
    test_equals(get_input_syntax_hint(term, 0, "postWhitespace"), " ");
    test_equals(get_input_syntax_hint(term, 1, "preWhitespace"), " ");

    term = parser::compile(&branch, parser::infix_expression, "5+3");
    test_assert(term->stringProp("syntax:preWhitespace") == "");
    test_equals(get_input_syntax_hint(term, 0, "postWhitespace"), "");
    test_equals(get_input_syntax_hint(term, 1, "preWhitespace"), "");
    test_assert(term->stringProp("syntax:postWhitespace") == "");
}

void test_list_arguments()
{
    Branch branch;
    Term *t = branch.compile("add(1 2 3)");
    test_assert(as_int(t->input(0)) == 1);
    test_assert(as_int(t->input(1)) == 2);
    test_assert(as_int(t->input(2)) == 3);

    t = branch.compile("add(5\n 6 , 7;8)");
    test_assert(as_int(t->input(0)) == 5);
    test_assert(as_int(t->input(1)) == 6);
    test_assert(as_int(t->input(2)) == 7);
    test_assert(as_int(t->input(3)) == 8);
}

void test_function_decl_parse_error()
{
    Branch branch;
    Term* t = branch.compile("def !@#$");

    test_assert(t->function == UNRECOGNIZED_EXPRESSION_FUNC);
    test_assert(has_static_error(t));
}

void test_semicolon_as_line_ending()
{
    Branch branch;
    branch.compile("1;2;3");
    test_assert(&branch);
    test_assert(branch.length() == 3);
    test_assert(is_value(branch[0]));
    test_assert(is_value(branch[1]));
    test_assert(is_value(branch[2]));
    test_assert(branch[0]->asInt() == 1);
    test_assert(branch[1]->asInt() == 2);
    test_assert(branch[2]->asInt() == 3);

    branch.clear();
    branch.compile("a = 1+2 ; b = mult(3,4) ; b -> print");
    test_assert(&branch);
    test_assert(branch.length() == 7);
    test_assert(branch["a"]->function->name == "add");
    test_assert(branch["b"]->function->name == "mult");

    branch.clear();
    branch.compile("cond = true; if cond { a = 1 } else { a = 2 }");
    branch.compile("a=a");

    test_assert(&branch);
    evaluate_branch(&branch);
    test_assert(branch.contains("a"));
    test_assert(branch["a"]->asInt() == 1);
    test_assert(branch.contains("cond"));
    set_bool(branch["cond"], false);
    evaluate_branch(&branch);
    test_assert(branch["a"]->asInt() == 2);
}

void test_unary_minus()
{
    Branch branch;
    Term* a = branch.eval("a = 1");
    Term* b = branch.eval("b = -a");

    test_assert(b->function->name == "neg");
    test_assert(b->input(0) == a);

    // - on a literal value should just modify that value, and not create a neg() operation.
    Term* c = branch.eval("-1");
    test_assert(is_value(c));
    test_assert(c->asInt() == -1);
    test_equals(get_term_source_text(c), "-1");

    // Sometimes, literals with a - sign are supposed to turn that into a minus operation
    // This is the case if there are no spaces around the -
    Term* d = branch.eval("2-1");
    test_assert(d->function->name == "sub");

    // Or if there are spaces on both sides of the -
    Term* e = branch.eval("2 - 1");
    test_assert(e->function->name == "sub");

    // But if there's a space before the - and not after it, that should be parsed as
    // two separate expressions.
    branch.clear();
    parser::compile(&branch, parser::statement_list, "2 -1");
    test_assert(branch.length() == 2);
    test_assert(is_int(branch[0]));
    test_assert(as_int(branch[0]) == 2);
    test_assert(is_int(branch[1]));
    test_assert(as_int(branch[1]) == -1);
}

void test_array_index_access()
{
    Branch branch;
    branch.compile("a = [1 2 3]");
    Term* b = branch.compile("a[0]");
    evaluate_branch(&branch);

    test_assert(b);
    test_assert(b->function == FUNCS.get_index);
    test_equals(b->asInt(), 1);
}

void test_namespace()
{
    Branch branch;
    Term* ns = branch.eval("namespace ns { a = 1; b = 2 }");

    test_assert(&branch);
    test_assert(ns->function == NAMESPACE_FUNC);
    test_assert(nested_contents(ns)->contains("a"));
    test_assert(nested_contents(ns)->contains("b"));

    Term* a = branch.eval("ns:a");
    test_assert(a->asInt() == 1);

    branch.clear();
    ns = branch.eval("namespace ns { def myfunc(int a) -> int { return(a+1) } }");
    Term* c = branch.eval("c = ns:myfunc(4)");
    test_assert(&branch);
    test_assert(c->asInt() == 5);
}

void test_subscripted_atom()
{
    Branch branch;

    branch.eval("a = 1");
    parser::compile(&branch, parser::atom_with_subscripts, "a.b.c");
}

void test_whitespace_after_statement()
{
    Branch branch;

    branch.eval("a = 1\n\n");

    // the 'a' term should have one newline after it, and the second newline
    // should be a comment line.
    test_assert(branch[0]->asInt() == 1);
    test_assert(branch[0]->name == "a");
    test_equals(branch[0]->stringProp("syntax:lineEnding"), "\n");
    test_assert(branch[1]->function == FUNCS.comment);
    test_equals(branch[1]->stringProp("comment"), "");
    test_equals(branch[1]->stringProp("syntax:lineEnding"), "\n");

    // Make sure that parser::statement only consumes one \n
    branch.clear();
    TokenStream tokens("a = 1\n\n");
    parser::ParserCxt context;
    Term* term = parser::statement(&branch, tokens, &context).term;
    test_assert(term->function == FUNCS.value);
    test_assert(term->name == "a");
    test_assert(tokens.nextIs(TK_NEWLINE));
    tokens.consume();
    test_assert(tokens.finished());
}

void test_significant_indentation()
{
    Branch branch;
    branch.eval("def func()\n"
                "  a = 1 + 2\n"
                "  b = a + 1\n"
                "c = 3 + 4");

    Branch* funcBranch = function_contents(branch["func"]);

    test_assert(funcBranch->get("a") != NULL);
    test_assert(funcBranch->get("b") != NULL);
    test_assert(funcBranch->get("c") == NULL);

    test_assert(branch.get("a") == NULL);
    test_assert(branch.get("b") == NULL);
    test_assert(branch.get("c") != NULL);
    test_assert(branch.get("c")->input(0)->owningBranch == &branch);
    test_assert(branch.get("c")->input(1)->owningBranch == &branch);

    branch.clear();

    // Test with whitespace before the function body
    branch.compile("def func() -> int\n"
                   "\n"
                   "  \n"
                   "    a = 2\n"
                   "    return(a)\n"
                   "b = 3");
    test_assert(branch.get("a") == NULL);
    test_assert(branch.get("b") != NULL);

    branch.clear();

    // Test with blank lines inside the function
    branch.eval("def func() -> int\n"
                "  a = 5\n"
                "\n"          // <-- Blank line doesn't have same indent
                "  return(6)\n"
                "b = 7");
    test_assert(&branch);
    test_assert(branch.get("a") == NULL);
    test_assert(branch.get("b") != NULL);

    branch.clear();

    // Test with no indented lines
    branch.compile("def func()\na = 5");
    test_assert(&branch);
    test_assert(branch["a"] != NULL);
    test_assert(branch["func"]->contents()->get("a") == NULL);
}

void test_significant_indentation2()
{
    Branch branch;

    // Test with indented comment line (comments should be ignored)
    branch.eval("def func()\n"
                "  a = 1\n"
                "    --comment\n"
                "  b = 2\n"
                "c = 3");
    test_assert(&branch);

    Branch* func = function_contents(branch.get("func"));
    test_assert(func->get("a") != NULL);
    test_assert(func->get("b") != NULL);
    test_assert(func->get("c") == NULL);
}

void test_significant_indentation_bug_with_empty_functions()
{
    Branch branch;
    branch.compile("namespace ns\n"
                   "  def a()\n"
                   "  def b()\n");

    // This once caused a bug where function b() was outside the namespace.
    test_assert(branch["ns"] != NULL);
    test_assert(branch["ns"]->contents()->get("a") != NULL);
    test_assert(branch["ns"]->contents()->get("b") != NULL);
    test_assert(branch["a"] == NULL);
    test_assert(branch["b"] == NULL);

    branch.clear();
    branch.compile("namespace ns\n"
                   "  def a() 'blah blah blah'\n"
                   "  def b() 'blah blah blah'\n");

    // This once caused a bug where function b() was outside the namespace.
    test_assert(branch["ns"] != NULL);
    test_assert(branch["ns"]->contents()->get("a") != NULL);
    test_assert(branch["ns"]->contents()->get("b") != NULL);
    test_assert(branch["a"] == NULL);
    test_assert(branch["b"] == NULL);
}

void test_sig_indent_one_liner()
{
    Branch branch;
    branch.eval("def f() 'avacado'\n  'burrito'\n'cheese'");
    Branch* f_contents = function_contents(branch["f"]);
    test_equals(f_contents->get(0)->asString(), "avacado");
    test_equals(branch[1], "burrito");
    test_equals(branch[2], "cheese");

    branch.clear();
    branch.eval("def g() 1 2 3\n  4");
    Branch* g_contents = function_contents(branch["g"]);
    test_equals(g_contents->get(0)->asInt(), 1);
    test_equals(g_contents->get(1)->asInt(), 2);
    test_equals(g_contents->get(2)->asInt(), 3);
    test_equals(branch[1]->asInt(), 4);
}

void test_sig_indent_bug_with_bad_func_header()
{
    Branch branch;
    branch.compile(
        "namespace ns\n"
        "  type T { int a, int b, int c }\n"
        "  def func1(state Font, string filename, int size) -> Font\n"
        "   'Something something something'\n"
        "\n");

    test_assert(branch["ns"] != NULL);
    test_assert(branch["ns"]->contents()->get("func1") != NULL);
    Term* func1 = branch["ns"]->contents()->get("func1");
    test_assert(is_function(func1));
}

void test_sig_indent_for_loop()
{
    Branch branch;
    branch.compile("for i in [1]; 1");
    test_assert(&branch);
}

void test_sig_indent_multiline_function()
{
    Branch branch;
#if 0
    TokenStream tokens("\n state number prev = val"
            "\n result = val - prev\n prev = val\n return result");
    std::cout << tokens.toString() << std::endl;

    Branch branch;
    parser::consume_branch(branch, tokens);
#endif

    branch.compile("def delta(number val) -> number\n state number prev = val"
            "\n result = val - prev\n prev = val\n return result");

    test_assert(branch.length() == 1);
    test_assert(branch["delta"]->contents()->length() > 5);
}

void test_sig_indent_nested_blocks()
{
    Branch branch;
    branch.compile(
        "namespace ns\n"
        "  def func1()\n"
        "  def func2()\n"
        "    if 1 == 2\n"
        "            b = sub(3 4)\n"
        "            a = add(3 4)\n"
        "    func1()\n"
        "  def func3()\n");

    test_assert(&branch);
    test_assert(branch["ns"]->contents()->get("func3") != NULL);
}

void test_sig_indent_bug_with_nested_one_liner()
{
    Branch branch;
    branch.compile(
            "def func()\n"
            "  if true; a = 1\n"
            "  b = 2\n");

    test_assert(&branch);
    test_assert(branch["b"] == NULL);
    test_assert(branch["func"]->contents()->get("b") != NULL);
}

void test_sig_indent_bug_with_for_loop_expression()
{
    Branch branch;
    branch.compile(
            "x = for i in 0..1\n"
            "  i + 5\n");

    evaluate_branch(&branch);
    test_equals(branch["x"], "[5]");
}

void test_namespace_with_curly_braces()
{
    Branch branch;
    branch.compile("namespace ns {\n"
                   "a = 1\n"
                   "}");
    test_assert(&branch);
    test_assert(branch["ns"] != NULL);
    test_assert(branch["ns"]->contents()->get("a") != NULL);
}

void test_statically_resolve_namespace_access()
{
    #if 0
    TEST_DISABLED
    Branch branch;
    Branch& ns1 = create_namespace(branch, "ns1");
    Term* a = create_int(ns1, 1, "a");
    Branch& ns2 = create_namespace(ns1, "ns2");
    Term* b = create_int(ns2, 1, "b");
    Branch& ns3 = create_namespace(ns2, "ns3");
    Term* c = create_int(ns3, 1, "c");

    Term* ns1_a = branch.compile("ns1:a");
    test_assert(is_namespace(ns1_a->input(0)));
    test_assert(parser::statically_resolve_namespace_access(ns1_a) == a);

    Term* ns1_ns2_b = branch.compile("ns1:ns2:b");
    test_assert(parser::statically_resolve_namespace_access(ns1_ns2_b) == b);

    Term* ns1_ns2_ns3_c = branch.compile("ns1:ns2:ns3:c");
    test_assert(parser::statically_resolve_namespace_access(ns1_ns2_ns3_c) == c);
    #endif
}

void test_get_number_of_decimal_figures()
{
    test_assert(parser::get_number_of_decimal_figures("1") == 0);
    test_assert(parser::get_number_of_decimal_figures("9438432") == 0);
    test_assert(parser::get_number_of_decimal_figures("1.") == 1);
    test_assert(parser::get_number_of_decimal_figures("1.1") == 1);
    test_assert(parser::get_number_of_decimal_figures("1.10") == 2);
    test_assert(parser::get_number_of_decimal_figures(".10") == 2);
    test_assert(parser::get_number_of_decimal_figures("0.101010") == 6);
}

void test_bug_with_nested_ifs()
{
    Branch branch;
    branch.compile("a = 'undef'\n"
                   "if true\n"
                   "  a = 'correct'\n"
                   "  if false\n"
                   "    a = 'very wrong'\n"
                   "else\n"
                   "  a = 'wrong'\n"
                   "a = a");

    test_assert(&branch);
    evaluate_branch(&branch);
    test_equals(branch["a"], "correct");
}

void test_source_location()
{
    Branch branch;
    branch.compile("a = 1");
    branch.compile("ra = ref(a)");
    Term* a = branch["a"];
    test_equals(a->sourceLoc.col, 0);
    test_equals(a->sourceLoc.line, 1);
    test_equals(a->sourceLoc.colEnd, 5);
    test_equals(a->sourceLoc.lineEnd, 1);

    branch.compile("def f()\n  b = add(1 2 3 4 5 6 7)\n  mult(3 4)\n");
    Term* b = branch["f"]->contents()->get("b");
    test_equals(b->sourceLoc.col, 2);
    test_equals(b->sourceLoc.line, 2);
    test_equals(b->sourceLoc.colEnd, 25);
    test_equals(b->sourceLoc.lineEnd, 2);
}

void register_tests()
{
    REGISTER_TEST_CASE(parser_tests::test_comment);
    REGISTER_TEST_CASE(parser_tests::test_blank_line);
    REGISTER_TEST_CASE(parser_tests::test_literal_integer);
    REGISTER_TEST_CASE(parser_tests::test_literal_float);
    REGISTER_TEST_CASE(parser_tests::test_literal_string);
    REGISTER_TEST_CASE(parser_tests::test_name_binding);
    REGISTER_TEST_CASE(parser_tests::test_function_call);
    REGISTER_TEST_CASE(parser_tests::test_identifier);
    REGISTER_TEST_CASE(parser_tests::test_rebind);
    REGISTER_TEST_CASE(parser_tests::test_infix);
    REGISTER_TEST_CASE(parser_tests::test_type_decl);
    REGISTER_TEST_CASE(parser_tests::test_function_decl);
    REGISTER_TEST_CASE(parser_tests::test_stateful_value_decl);
    REGISTER_TEST_CASE(parser_tests::test_arrow_concatenation);
    REGISTER_TEST_CASE(parser_tests::test_arrow_concatenation2);
    REGISTER_TEST_CASE(parser_tests::test_dot_concatenation);
    REGISTER_TEST_CASE(parser_tests::test_syntax_hints);
    REGISTER_TEST_CASE(parser_tests::test_implicit_copy_by_identifier);
    REGISTER_TEST_CASE(parser_tests::test_rebinding_infix_operator);
    REGISTER_TEST_CASE(parser_tests::test_infix_whitespace);
    REGISTER_TEST_CASE(parser_tests::test_list_arguments);
    REGISTER_TEST_CASE(parser_tests::test_function_decl_parse_error);
    REGISTER_TEST_CASE(parser_tests::test_semicolon_as_line_ending);
    REGISTER_TEST_CASE(parser_tests::test_unary_minus);
    REGISTER_TEST_CASE(parser_tests::test_array_index_access);
    //TEST_DISABLED REGISTER_TEST_CASE(parser_tests::test_namespace);
    REGISTER_TEST_CASE(parser_tests::test_subscripted_atom);
    REGISTER_TEST_CASE(parser_tests::test_whitespace_after_statement);
    REGISTER_TEST_CASE(parser_tests::test_significant_indentation);
    REGISTER_TEST_CASE(parser_tests::test_significant_indentation2);
    REGISTER_TEST_CASE(parser_tests::test_significant_indentation_bug_with_empty_functions);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_one_liner);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_bug_with_bad_func_header);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_for_loop);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_multiline_function);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_nested_blocks);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_bug_with_nested_one_liner);
    REGISTER_TEST_CASE(parser_tests::test_sig_indent_bug_with_for_loop_expression);
    REGISTER_TEST_CASE(parser_tests::test_namespace_with_curly_braces);
    REGISTER_TEST_CASE(parser_tests::test_statically_resolve_namespace_access);
    REGISTER_TEST_CASE(parser_tests::test_get_number_of_decimal_figures);
    REGISTER_TEST_CASE(parser_tests::test_bug_with_nested_ifs);
    REGISTER_TEST_CASE(parser_tests::test_source_location);
}

} // namespace parser_tests
} // namespace circa
