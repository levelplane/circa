// Copyright 2009 Paul Hodge

#ifndef CIRCA_TRAINING_INCLUDED
#define CIRCA_TRAINING_INCLUDED

#include "common_headers.h"

namespace circa {

bool is_trainable(Term* term);
void update_derived_trainables(Branch& branch);
void generate_training(Branch& branch, Term* subject, Term* desired);

} // namespace circa

#endif