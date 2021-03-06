// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <circa_internal.h>

namespace circa {
namespace branch_iterator_tests {

void test_simple()
{
    Branch branch;

    Term* a = branch.compile("a = 1");
    Term* b = branch.compile("b = 2");

    test_assert(a);
    test_assert(b);

    BranchIterator it(&branch);

    test_assert(it.current() == a);
    it.advance();
    test_assert(it.current() == b);
    it.advance();
    test_assert(it.finished());

    Term* sub = branch.compile("def func() { c = 3; d = 4 }");

    it.reset(&branch);

    test_assert(it.current() == a);
    it.advance();
    test_assert(it.current() == b);
    it.advance();
    test_assert(it.current() == sub);
    it.advance();
    test_assert(it.current()->name == "c");
    it.advance();
    test_assert(it.current()->name == "d");
    it.advanceSkippingBranch();
    test_assert(it.current()->function->name == "output_placeholder");
    it.advance();
    test_assert(it.finished());
}

void iterate_names_to_list(UpwardIterator it, List& list)
{
    list.clear();
    for ( ; it.unfinished(); it.advance())
        set_string(list.append(), it->name);
}

void test_upwards_iterator()
{
    Branch branch;
    branch.compile("a = 1; b = 2; c = { d = 3; e = 4} f = 5; g = { h = 6; i = 7; j = 8} k = 9");

    List names;
    iterate_names_to_list(UpwardIterator(branch["g"]->contents("j")), names);
    test_equals(&names, "['i', 'h', 'g', 'f', 'c', 'b', 'a']");
}

void test_upwards_iterator_nulls()
{
    Branch branch;
    branch.compile("a = 1; b = 2");
    branch.append(NULL);
    branch.compile("c = 1; d = 2");

    List names;
    iterate_names_to_list(UpwardIterator(branch["d"]), names);
    test_equals(&names, "['c', 'b', 'a']");

    branch.clear();
    branch.compile("a = 2; b = {}");
    branch["b"]->nestedContents->append(NULL);
    branch["b"]->nestedContents->compile("c = 3");
    branch["b"]->nestedContents->compile("d = {}");
    branch["b"]->contents("d")->nestedContents->append(NULL);

    Term* e = branch["b"]->contents("d")->nestedContents->compile("e = 1");
    iterate_names_to_list(UpwardIterator(e), names);
    test_equals(&names, "['d', 'c', 'b', 'a']");
}

void test_input_iterator()
{
    Branch branch;

    branch.compile("a = 1");
    branch.compile("b = 2");
    branch.compile("c = add_i(a b)");
    branch.compile("nullthis = 1");
    branch.setNull(3);
    Term* d = branch.compile("d = add_i(a b c)");
    set_input(d, 1, NULL);

    BranchInputIterator it(&branch);
    test_assert(it.currentTerm()->name == "c");
    test_assert(it.currentInput()->name == "a");
    it.advance();
    test_assert(it.currentTerm()->name == "c");
    test_assert(it.currentInput()->name == "b");
    it.advance();
    test_assert(it.currentTerm()->name == "d");
    test_assert(it.currentInput()->name == "a");
    it.advance();
    test_assert(it.currentTerm()->name == "d");
    test_assert(it.currentInput()->name == "c");
    it.advance();
    test_assert(it.finished());
}

void test_outer_input_iterator()
{
    Branch branch;
    branch.compile("a = 1");
    branch.compile("b = 2");
    Branch* inner = branch.compile("inner = branch()")->contents();
    inner->compile("c = 3");
    inner->compile("d = add_i(a b c)");
    inner->compile("e = add_i(c)");
    inner->compile("f = add_i(c a)");

    OuterInputIterator it(inner);
    test_assert(it.currentTerm()->name == "d");
    test_assert(it.currentInput()->name == "a");
    it.advance();
    test_assert(it.currentTerm()->name == "d");
    test_assert(it.currentInput()->name == "b");
    it.advance();
    test_assert(it.currentTerm()->name == "f");
    test_assert(it.currentInput()->name == "a");
    it.advance();
    test_assert(it.finished());
}

void register_tests()
{
    REGISTER_TEST_CASE(branch_iterator_tests::test_simple);
    REGISTER_TEST_CASE(branch_iterator_tests::test_upwards_iterator);
    REGISTER_TEST_CASE(branch_iterator_tests::test_upwards_iterator_nulls);
    REGISTER_TEST_CASE(branch_iterator_tests::test_input_iterator);
    REGISTER_TEST_CASE(branch_iterator_tests::test_outer_input_iterator);
}

} // namespace branch_iterator_tests
} // namespace circa
