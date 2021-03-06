// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <ctime>

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace rand_function {

    bool seeded = false;

    void seed_if_needed()
    {
        if (!seeded) {
            srand((unsigned) time(0));
            seeded = true;
        }
    }

    CA_FUNCTION(evaluate_f)
    {
        seed_if_needed();
        set_float(OUTPUT, (float) rand() / RAND_MAX);
    }

    CA_FUNCTION(evaluate_f_range)
    {
        seed_if_needed();
        float r = (float) rand() / RAND_MAX;
        float min = INPUT(0)->toFloat();
        float max = INPUT(1)->toFloat();

        if (min >= max) {
            RAISE_ERROR("min is >= max");
            return;
        }

        set_float(OUTPUT, min + r * (max - min));
    }

    CA_FUNCTION(evaluate_i)
    {
        seed_if_needed();
        set_int(OUTPUT, rand());
    }

    CA_FUNCTION(evaluate_i_i)
    {
        seed_if_needed();
        int period = INPUT(0)->asInt();

        // TODO: replace this, builtin rand() does not have good randomness in lower bits.
        set_int(OUTPUT, rand() % period);
    }

    void setup(Branch* kernel)
    {
        import_function(kernel, evaluate_f, "rand() -> number");
        import_function(kernel, evaluate_f_range,
                "rand_range(number min,number max) -> number");
        import_function(kernel, evaluate_i, "rand_i() -> int");
    }
}
} // namespace circa
