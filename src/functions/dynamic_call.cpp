// Copyright (c) 2007-2010 Andrew Fischer. All rights reserved

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace dynamic_call_function {

    void dynamic_call(caStack* stack)
    {
        // Grab function & inputs
        Function* func = as_function(circa_input(stack, 0));
        Value inputs;
        circa_swap(circa_input(stack, 1), &inputs);

        // Pop calling frame
        pop_frame(stack);

        // Replace it with the callee frame
        push_frame_with_inputs(stack, function_contents(func), &inputs);

#if 0 // Old version
        Term* function = as_function_pointer(INPUT(0));
        List* inputs = List::checkCast(INPUT(1));

        Term temporaryTerm;
        temporaryTerm.function = function;
        temporaryTerm.type = CALLER->type;

        List* stack = &CONTEXT->stack;
        List* frame = List::cast(stack->append(), 0);

        int numInputs = inputs->length();
        int numOutputs = 1;

        frame->resize(numInputs + numOutputs);
        temporaryTerm.inputIsns.inputs.resize(numInputs);
        temporaryTerm.inputIsns.outputs.resize(numOutputs);

        // Populate input instructions, use our stack frame.
        int frameIndex = 0;
        for (int i=0; i < numInputs; i++) {
            copy(inputs->get(i), frame->get(frameIndex));

            InputInstruction* isn = &temporaryTerm.inputIsns.inputs[i];
            isn->type = InputInstruction::LOCAL;
            isn->data.index = frameIndex;
            isn->data.relativeFrame = 0;
            frameIndex++;
        }

        int outputIndex = frameIndex;

        for (int i=0; i < numOutputs; i++) {
            InputInstruction* isn = &temporaryTerm.inputIsns.outputs[i];
            isn->type = InputInstruction::LOCAL;
            isn->data.index = frameIndex;
            isn->data.relativeFrame = 0;
            frameIndex++;
        }

        // Evaluate
        evaluate_single_term(CONTEXT, &temporaryTerm);

        // Save the stack frame and pop. (the OUTPUT macro isn't valid until
        // we restore the stack to its original size).
        Value finishedFrame;
        swap(frame, &finishedFrame);
        stack->pop();

        swap(list_get(&finishedFrame, outputIndex), OUTPUT);
#endif
    }

    void setup(Branch* kernel)
    {
        import_function(kernel, dynamic_call,  "dynamic_call(Function f, List args)");
    }
}
}
