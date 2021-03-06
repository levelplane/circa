// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <circa_internal.h>

namespace circa {
namespace float_tests {

void test_cast()
{
    caValue f;
    set_float(&f, 5.0);
    caValue s;
    set_string(&s, "hello");


    test_assert(cast_possible(&f, &FLOAT_T));
    test_assert(!cast_possible(&s, &FLOAT_T));

    caValue f2;
    test_assert(cast(&f, &FLOAT_T, &f2));

    test_assert(equals(&f, &f2));
}

void register_tests()
{
    REGISTER_TEST_CASE(float_tests::test_cast);
}

} // namespace float_tests
} // namespace circa
