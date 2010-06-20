// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "plastic.h"

#include "ide.h"
#include "text.h"

using namespace circa;

// the filename of this binary, passed in as args[0]
std::string BINARY_NAME;

Branch* RUNTIME_BRANCH = NULL;
Branch* USERS_BRANCH = NULL;

bool CONTINUE_MAIN_LOOP = true;

Term* TIME = NULL;
Term* TIME_DELTA = NULL;
Term* RENDER_DURATION = NULL;
long PREV_SDL_TICKS = 0;
int TARGET_FPS = 60;

bool PAUSED = false;
PauseReason PAUSE_REASON;

void error(std::string const& msg)
{
    std::cout << "error: " << msg << std::endl;
}

circa::Branch& runtime_branch()
{
    return *RUNTIME_BRANCH;
}

circa::Branch& users_branch()
{
    return *USERS_BRANCH;
}

std::string get_home_directory()
{
    char* circa_home = getenv("CIRCA_HOME");
    if (circa_home == NULL) {
        return get_directory_for_filename(BINARY_NAME);
    } else {
        return std::string(circa_home) + "/plastic";
    }
}

std::string find_runtime_file()
{
    return get_home_directory() + "/runtime/main.ca";
}

std::string find_asset_file(std::string const& filename)
{
    return get_home_directory() + "/" + filename;
}

bool load_runtime()
{
    // Pre-setup
    text::pre_setup(runtime_branch());

    // Load runtime.ca
    std::string runtime_ca_path = find_runtime_file();
    if (!file_exists(runtime_ca_path)) {
        std::cerr << "fatal: Couldn't find runtime.ca file. (expected at "
            << runtime_ca_path << ")" << std::endl;
        return false;
    }
    parse_script(runtime_branch(), runtime_ca_path);

    assert(branch_check_invariants(runtime_branch(), &std::cout));

    // Fetch constants
    TIME = procure_value(runtime_branch(), FLOAT_TYPE, "time");
    TIME_DELTA = procure_value(runtime_branch(), FLOAT_TYPE, "time_delta");
    RENDER_DURATION = procure_value(runtime_branch(), FLOAT_TYPE, "render_duration");

    return true;
}

bool initialize_plastic()
{
    RUNTIME_BRANCH = &create_branch(*circa::KERNEL, "plastic_main");
    return load_runtime();
}

bool setup_builtin_functions()
{
    postprocess_functions::setup(runtime_branch());
    ide::setup(runtime_branch());
    image::setup(runtime_branch());
    input::setup(runtime_branch());
    mesh::setup(runtime_branch());
    gl_shapes::setup(runtime_branch());
    textures::setup(runtime_branch());
    text::setup(runtime_branch());

    if (has_static_errors(runtime_branch())) {
        print_static_errors_formatted(runtime_branch(), std::cout);
        std::cout << std::endl;
        return false;
    }

    return true;
}

bool reload_runtime()
{
    runtime_branch().clear();
    if (!load_runtime())
        return false;
    if (!setup_builtin_functions())
        return false;

    // Write window width & height
    set_int(runtime_branch()["window"]->getField("width"), WINDOW_WIDTH);
    set_int(runtime_branch()["window"]->getField("height"), WINDOW_HEIGHT);

    return true;
}

bool evaluate_main_script()
{
    EvalContext context;
    evaluate_branch(&context, runtime_branch());

    if (context.errorOccurred) {
        std::cout << "Runtime error:" << std::endl;
        print_runtime_error_formatted(context, std::cout);
        std::cout << std::endl;
        PAUSED = true;
        PAUSE_REASON = RUNTIME_ERROR;
        return false;
    }

    return true;
}

bool load_user_script_filename(std::string const& _filename)
{
    Term* users_branch = runtime_branch()["users_branch"];
    USERS_BRANCH = &users_branch->asBranch();

    if (_filename != "") {
        std::string filename = get_absolute_path(_filename);

        Term* user_script_filename = runtime_branch().findFirstBinding("user_script_filename");
        set_str(user_script_filename, filename);
        mark_stateful_value_assigned(user_script_filename);
        std::cout << "Loading script: " << filename << std::endl;
    }
        
    return true;
}

void main_loop()
{
    input::capture_events();

    long ticks = SDL_GetTicks();

    gl_clear_error();

    // Evaluate script
    if (PAUSED) {
        set_float(TIME_DELTA, 0.0f);
    } else {
        set_float(TIME, ticks / 1000.0f);
        set_float(TIME_DELTA, (ticks - PREV_SDL_TICKS) / 1000.0f);
    }
    PREV_SDL_TICKS = ticks;

    render_frame();

    long new_ticks = SDL_GetTicks();

    set_float(RENDER_DURATION, (new_ticks - ticks));

    // Delay to limit framerate
    const long ticks_per_second = long(1.0 / TARGET_FPS * 1000);
    if ((new_ticks - ticks) < ticks_per_second) {
        long delay = ticks_per_second - (new_ticks - ticks);
        SDL_Delay(delay);
    }
}

int plastic_main(std::vector<std::string> args)
{
    circa::initialize();
    if (!initialize_plastic()) return 1;

    std::string arg0;
    if (args.size() > 0) arg0 = args[0];

    // -gd to generate docs
    if (arg0 == "-gd") {
        std::cout << "writing docs to " << args[1] << std::endl;
        std::stringstream out;
        generate_plastic_docs(out);
        write_text_file(args[1], out.str());
        return 0;
    }

    // -p to print raw compiled code
    if (arg0 == "-p") {
        if (!load_user_script_filename(args[1]))
            return 1;

        EvalContext cxt;
        include_function::load_script(&cxt, users_branch().owningTerm);

        print_branch_raw(std::cout, users_branch());
        return 0;
    }

    // -tr to do a test run of a script, without creating a display.
    if (arg0 == "-tr") {
        if (!load_user_script_filename(args[1]))
            return 1;

        if (has_static_errors(users_branch())) {
            print_static_errors_formatted(users_branch(), std::cout);
            return 1;
        }

        for (int i=0; i < 5; i++)
            if (!evaluate_main_script())
                return 1;

        return 0;
    }

    // Normal operation, load the script file in argument 0.
    std::string filename = arg0;

    if (!load_user_script_filename(filename))
        return 1;

    if (has_static_errors(users_branch())) {
        print_static_errors_formatted(users_branch(), std::cout);
        return 1;
    }

    if (!setup_builtin_functions()) return 1;
    if (!initialize_display()) return 1;

    PREV_SDL_TICKS = SDL_GetTicks();

    // Main loop
    while (CONTINUE_MAIN_LOOP)
        main_loop();

    // Quit SDL
    SDL_Quit();

    return 0;
}
