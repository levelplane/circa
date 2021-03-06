// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#pragma once

namespace circa {

struct World {
    circa::Value actorList;
    caStack* actorStack;

    circa::Value freeModules;

protected:
    // Disallow C++ construction
    World();
    ~World();
};

World* alloc_world();
void actor_send_message(ListData* actor, caValue* message);
void actor_run_message(caStack* stack, ListData* actor, caValue* message);
ListData* find_actor(World* world, const char* name);

// A 'free' module is one that doesn't live at the top-level, and it doesn't have a name.
Branch* create_free_module(caWorld* world);

void refresh_all_modules(caWorld* world);

} // namespace circa
