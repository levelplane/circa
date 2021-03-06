// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "circa/circa.h"

#include "framework.h"

#include "../../src/branch.h"
#include "../../src/building.h"
#include "../../src/modules.h"
#include "../../src/metaprogramming.h"

using namespace circa;

void translate_terms()
{
    Branch branch1;
    Branch branch2;

    branch1.compile("a = 1");
    Term* b1 = branch1.compile("b = 2");
    branch1.compile("c = 3");

    branch2.compile("a = 1");
    Term* b2 = branch2.compile("b = 2");
    branch2.compile("c = 3");

    test_assert(b2 == translate_term_across_branches(b1, &branch1, &branch2));
}

void update_references()
{
    Branch library1;
    library1.compile("def f()");
    Branch library2;
    library2.compile("def f()");

    Branch target;
    Term* call = target.compile("f()");
    change_function(call, library1.get("f"));
    test_assert(call->function == library1.get("f"));

    update_all_code_references(&target, &library1, &library2);

    test_assert(call->function != library1.get("f"));
    test_assert(call->function == library2.get("f"));
}

void migration_register_tests()
{
    REGISTER_TEST_CASE(translate_terms);
    REGISTER_TEST_CASE(update_references);
}
