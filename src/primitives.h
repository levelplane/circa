// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#ifndef CIRCA_PRIMITIVES_INCLUDED
#define CIRCA_PRIMITIVES_INCLUDED

#include "common_headers.h"

namespace circa {

// for Function:
typedef void (*EvaluateFunc)(EvalContext* context, Term* caller);
typedef Term* (*SpecializeTypeFunc)(Term* caller);

typedef std::string (*ToSourceStringFunc)(Term* term);
typedef bool (*CheckInvariantsFunc)(Term* term, std::string* output);

extern Term* ANY_TYPE;
extern Term* BOOL_TYPE;
extern Term* FLOAT_TYPE;
extern Term* INT_TYPE;
extern Term* REF_TYPE;
extern Term* STRING_TYPE;

// for Function:
extern Term* EVALUATE_THUNK_TYPE;
extern Term* SPECIALIZE_THUNK_TYPE;
extern Term* TO_STRING_THUNK_TYPE;
extern Term* CHECK_INVARIANTS_THUNK_TYPE;

// for Function:
EvaluateFunc& as_evaluate_thunk(Term*);
SpecializeTypeFunc& as_specialize_type_thunk(Term*);
ToSourceStringFunc& as_to_source_string_thunk(Term*);
CheckInvariantsFunc& as_check_invariants_thunk(Term*);

void initialize_primitive_types(Branch& kernel);

// Do some more setup, after all the standard builtin types have been created.
void post_setup_primitive_types();

} // namespace circa

#endif // CIRCA_PRIMITIVES_INCLUDED
