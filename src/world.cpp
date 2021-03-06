// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "common_headers.h"

#include "circa/circa.h"

#include "branch.h"
#include "building.h"
#include "code_iterators.h"
#include "evaluation.h"
#include "kernel.h"
#include "list.h"
#include "metaprogramming.h"
#include "modules.h"
#include "string_type.h"
#include "tagged_value.h"
#include "term.h"
#include "world.h"

namespace circa {

World* alloc_world()
{
    World* world = (World*) malloc(sizeof(*world));

    initialize_null(&world->actorList);
    set_list(&world->actorList, 0);

    world->actorStack = circa_alloc_stack(world);

    initialize_null(&world->freeModules);
    set_list(&world->freeModules, 0);

    return world;
}

ListData* find_actor(World* world, const char* name)
{
    caValue* actors = &world->actorList;
    for (int i=0; i < list_length(actors); i++) {
        caValue* actor = list_get(actors, i);
        if (string_eq(list_get(actor, 0), name))
            return as_list_data(actor);
    }

    // Not found, try to load it
    caValue* actor = circa_actor_new_from_module(world, name, name);

    if (actor != NULL)
        return as_list_data(actor);
    
    return NULL;
}

void actor_send_message(ListData* actor, caValue* message)
{
    caValue* queue = list_get(actor, 2);

    copy(message, list_append(queue));
}

void actor_run_message(caStack* stack, ListData* actor, caValue* message)
{
    Branch* branch = find_loaded_module(as_cstring(list_get(actor, 1)));
    Frame* frame = push_frame(stack, branch);

    frame_set_stop_when_finished(frame);
    copy(message, circa_input(stack, 0));

    // Copy state (if any)
    Term* state_in = find_state_input(branch);
    if (state_in != NULL)
        copy(list_get(actor, 3), get_register(stack, state_in));

    run_interpreter(stack);

    // Preserve state, if found, and if there was no error.
    Term* state_out = find_state_output(branch);
    if (!error_occurred(stack) && state_out != NULL) {
        copy(get_register(stack, state_out), list_get(actor, 3));
    }

    // Do something with an error. TODO is a more robust way of saving errors.
    if (error_occurred(stack)) {
        caValue* actorName = list_get(actor, 0);
        std::cout << "Error occured in actor " << as_string(actorName)
            << " with message: " << to_string(message) << std::endl;
        circa_print_error_to_stdout(stack);
    }

    circa_clear_stack(stack);
}

int actor_run_queue(caStack* stack, ListData* actor, int maxMessages)
{
    caValue* messages = list_get(actor, 2);

    // Iterate once for each message
    // Note that new messages might be appended to the queue while we are running;
    // don't run the new messages right now.

    int count = circa_count(messages);
    if (count == 0)
        return 0;

    // Enforce maximum messages per call
    if (maxMessages > 0 && count >= maxMessages)
        count = maxMessages;

    for (int i=0; i < count; i++) {
        caValue* message = list_get(messages, i);

        actor_run_message(stack, actor, message);
    }

    // Remove the messages that we handled
    Value newQueue;
    list_slice(messages, count, -1, &newQueue);
    swap(&newQueue, messages);

    return count;
}

Branch* create_free_module(caWorld* world)
{
    Branch* branch = alloc_branch_gc();
    set_branch(list_append(&world->freeModules), branch);
    return branch;
}

void update_branch_after_module_reload(Branch* target, Branch* oldBranch, Branch* newBranch)
{
    // Noop if the target is our new branch
    if (target == newBranch)
        return;

    ca_assert(target != oldBranch);

    update_all_code_references(target, oldBranch, newBranch);
}

void update_world_after_module_reload(caWorld* world, Branch* oldBranch, Branch* newBranch)
{
    // Update references in every module
    for (BranchIteratorFlat it(kernel()); it.unfinished(); it.advance()) {
        Term* term = it.current();
        if (term->function == FUNCS.imported_file) {
            update_branch_after_module_reload(term->nestedContents, oldBranch, newBranch);
        }
    }

    // Update references in free modules
    for (int i=0; i < list_length(&world->freeModules); i++) {
        caValue* container = list_get(&world->freeModules, i);
        Branch* existing = as_branch(container);
        update_branch_after_module_reload(existing, oldBranch, newBranch);
    }

    // Update top-level state
    for (int i=0; i < list_length(&world->actorList); i++) {
        caValue* actor = list_get(&world->actorList, i);
        caValue* state = list_get(actor, 3);
        update_all_code_references_in_value(state, oldBranch, newBranch);
    }
}

void refresh_all_modules(caWorld* world)
{
    // Iterate over top-level modules
    for (BranchIteratorFlat it(kernel()); it.unfinished(); it.advance()) {
        Term* term = it.current();
        if (term->function == FUNCS.imported_file) {
            
            Branch* existing = term->nestedContents;
            Branch* latest = load_latest_branch(existing);

            if (existing != latest) {
                term->nestedContents = latest;

                update_world_after_module_reload(world, existing, latest);
            }
        }
    }

    // Iterate over free modules
    for (int i=0; i < list_length(&world->freeModules); i++) {
        caValue* container = list_get(&world->freeModules, i);
        Branch* existing = as_branch(container);
        Branch* latest = load_latest_branch(existing);

        if (existing != latest) {
            set_branch(container, latest);

            update_world_after_module_reload(world, existing, latest);
        }
    }
}

} // namespace circa

using namespace circa;

void circa_actor_new_from_file(caWorld* world, const char* actorName, const char* filename)
{
    load_module_from_file(actorName, filename);

    caValue* actor = list_append(&world->actorList);
    create(TYPES.actor, actor);

    // Actor has shape:
    // { String name, String moduleName, List incomingQueue, any stateVal }
    set_string(list_get(actor, 0), actorName);
    set_string(list_get(actor, 1), actorName);
}

caValue* circa_actor_new_from_module(caWorld* world, const char* actorName, const char* moduleName)
{
    Branch* module = load_module(moduleName, NULL);

    if (module == NULL) {
        return NULL;
    }

    caValue* actor = list_append(&world->actorList);
    create(TYPES.actor, actor);

    // Actor has shape:
    // { String name, String moduleName, List incomingQueue, any stateVal }
    set_string(list_get(actor, 0), actorName);
    set_string(list_get(actor, 1), actorName);

    return actor;
}

void circa_actor_post_message(caWorld* world, const char* actorName, caValue* message)
{
    ListData* actor = find_actor(world, actorName);
    if (actor == NULL) {
        printf("couldn't find actor named: %s\n", actorName);
        return;
    }

    actor_send_message(actor, message);
}

void circa_actor_run_message(caWorld* world, const char* actorName, caValue* message)
{
    // Refresh all scripts when starting a top-level call
    refresh_all_modules(world);

    caStack* stack = world->actorStack;

    ListData* actor = find_actor(world, actorName);
    if (actor == NULL) {
        printf("couldn't find actor named: %s\n", actorName);
        return;
    }
    actor_run_message(stack, actor, message);
}

int circa_actor_run_queue(caWorld* world, const char* actorName, int maxMessages)
{
    // Refresh all scripts when starting a top-level call
    refresh_all_modules(world);

    caStack* stack = world->actorStack;
    ca_assert(world != NULL);

    ListData* actor = find_actor(world, actorName);

    if (actor == NULL) {
        printf("couldn't find actor named: %s\n", actorName);
        return 0;
    }

    return actor_run_queue(stack, actor, maxMessages);
}

int circa_actor_run_all_queues(caWorld* world, int maxMessages)
{
    // Refresh all scripts when starting a top-level call
    refresh_all_modules(world);

    caStack* stack = world->actorStack;

    int handledCount = 0;

    for (int i=0; i < list_length(&world->actorList); i++) {
        handledCount += actor_run_queue(stack, as_list_data(list_get(&world->actorList, i)),
            maxMessages);
    }

    return handledCount;
}

void circa_actor_clear_all(caWorld* world)
{
    set_list(&world->actorList, 0);
}
