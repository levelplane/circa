// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace add_function {

    CA_FUNCTION(add_i_evaluate)
    {
        int sum = circa_int_input(STACK, 0) + circa_int_input(STACK, 1);
        set_int(circa_output(STACK, 0), sum);
    }

    CA_FUNCTION(add_f_evaluate)
    {
        float sum = circa_float_input(STACK, 0) + circa_float_input(STACK, 1);
        set_float(circa_output(STACK, 0), sum);
    }

#if 0
    CA_FUNCTION(add_dynamic)
    {
        bool allInts = true;
        for (int i=0; i < NUM_INPUTS; i++) {
            if (!is_int(INPUT(0)))
                allInts = false;
        }

        if (allInts)
            add_i_evaluate(_cxt, _ins, _outs);
        else
            add_f_evaluate(_cxt, _ins, _outs);
    }
#endif

    CA_FUNCTION(add_feedback)
    {
        #if 0
        OLD_FEEDBACK_IMPL_DISABLED
        Term* target = INPUT_TERM(0);
        float desired = FLOAT_INPUT(1);

        float delta = desired - to_float(target);

        Branch* outputList = feedback_output(CALLER);
        for (int i=0; i < outputList.length(); i++) {
            Term* output = outputList[i];
            Term* outputTarget = target->input(i);
            float balanced_delta = delta * get_feedback_weight(output);
            set_float(output, to_float(outputTarget) + balanced_delta);
        }
        #endif
    }

    void setup(Branch* kernel)
    {
        FUNCS.add_i = import_function(kernel, add_i_evaluate, "add_i(int, int) -> int");
        FUNCS.add_f = import_function(kernel, add_f_evaluate, "add_f(number, number) -> number");
    }
} // namespace add_function
} // namespace circa
