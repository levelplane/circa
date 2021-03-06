// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "common_headers.h"

#include "branch.h"
#include "building.h"
#include "kernel.h"
#include "codegen.h"
#include "evaluation.h"
#include "feedback.h"
#include "file_utils.h"
#include "introspection.h"
#include "list.h"
#include "modules.h"
#include "parser.h"
#include "source_repro.h"
#include "static_checking.h"
#include "string_type.h"
#include "term.h"

#include "tools/build_tool.h"
#include "tools/command_reader.h"
#include "tools/generate_cpp.h"
#include "tools/debugger_repl.h"
#include "tools/exporting_parser.h"
#include "tools/file_checker.h"
#include "tools/repl.h"

namespace circa {

void print_usage()
{
    std::cout <<
        "Usage:\n"
        "  circa <options> <filename>\n"
        "  circa <options> <dash-command> <command args>\n"
        "\n"
        "Available options:\n"
        "  -libpath <path>     : Add a module search path\n"
        "  -p                  : Print out raw source\n"
        "  -pp                 : Print out raw source with properties\n"
        "  -n                  : Don't actually run the script (for use with -p, -pp or -s)\n"
        "  -break-on <id>      : Debugger break when term <id> is created\n"
        "  -print-state        : Print state as text after running the script\n"
        "\n"
        "Available commands:\n"
        "  -loop <filename>  : Run the script repeatedly until terminated\n"
        "  -call <filename> <func name> <args>\n"
        "                    : Call a function in a script file, print results\n"
        "  -repl             : Start an interactive read-eval-print-loop\n"
        "  -e <expression>   : Evaluate an expression on the command line\n"
        "  -check <filename> : Statically check the script for any errors\n"
        "  -build <dir>      : Rebuild a module using a build.ca file\n"
        "  -run-stdin        : Read and execute commands from stdin\n"
        "  -source-repro     : Compile and reproduce a script's source (for testing)\n"
        << std::endl;
}

int run_command_line(caWorld* world, caValue* args)
{
    bool printRaw = false;
    bool printRawWithProps = false;
    bool printState = false;
    bool dontRunScript = false;
    bool printTrace = false;

    // Prepended options
    while (true) {

        if (list_length(args) == 0)
            break;

        if (string_eq(list_get(args, 0), "-break-on")) {
            String name;
            DEBUG_BREAK_ON_TERM = atoi(as_cstring(list_get(args, 1)));

            list_remove_index(args, 0);
            list_remove_index(args, 0);
            std::cout << "breaking on creation of term: " << DEBUG_BREAK_ON_TERM << std::endl;
            continue;
        }

        if (string_eq(list_get(args, 0), "-libpath")) {
            // Add a module path
            modules_add_search_path(as_cstring(list_get(args, 1)));
            list_remove_index(args, 0);
            list_remove_index(args, 0);
            continue;
        }

        if (string_eq(list_get(args, 0), "-p")) {
            printRaw = true;
            list_remove_index(args, 0);
            continue;
        }

        if (string_eq(list_get(args, 0), "-pp")) {
            printRawWithProps = true;
            list_remove_index(args, 0);
            continue;
        }

        if (string_eq(list_get(args, 0), "-n")) {
            dontRunScript = true;
            list_remove_index(args, 0);
            continue;
        }
        if (string_eq(list_get(args, 0), "-print-state")) {
            printState = true;
            list_remove_index(args, 0);
            continue;
        }
        if (string_eq(list_get(args, 0), "-t")) {
            printTrace = true;
            list_remove_index(args, 0);
            continue;
        }

        break;
    }

    // No arguments remaining
    if (list_length(args) == 0) {
#ifdef CIRCA_TEST_BUILD
        run_all_tests();
#else
        print_usage();
#endif
        return 0;
    }

    // Check to handle args[0] as a dash-command.

    // Print help
    if (string_eq(list_get(args, 0), "-help")) {
        print_usage();
        return 0;
    }

    // Eval mode
    if (string_eq(list_get(args, 0), "-e")) {
        list_remove_index(args, 0);

        String command;

        bool firstArg = true;
        while (!list_empty(args)) {
            if (!firstArg)
                string_append(&command, " ");
            string_append(&command, list_get(args, 0));
            list_remove_index(args, 0);
            firstArg = false;
        }

        Branch workspace;
        caValue* result = term_value(workspace.eval(as_cstring(&command)));
        std::cout << result->toString() << std::endl;
        return 0;
    }

    // Start repl
    if (string_eq(list_get(args, 0), "-repl"))
        return run_repl();

    // Start evaluation loop
#if 0
    if (string_eq(list_get(args, 0), "-loop")) {
        Branch branch;
        load_script(&branch, as_cstring(list_get(args, 1)));

        Stack* stack = alloc_stack(NULL);

        while (true) {
            if (error_occurred(stack)) {
                print_error_stack(stack, std::cout);
                return -1;
            }

            evaluate_branch(stack, &branch);

            // Sleep for 1 second before next iteration. This is silly, it should either
            // iterate more quickly or have a smarter way to know when to loop.
            sleep(1);
        }
    }
#endif

    if (string_eq(list_get(args, 0), "-call")) {
        Branch* branch = create_branch(kernel());
        Name loadResult = load_script(branch, as_cstring(list_get(args, 1)));

        if (loadResult == name_Failure) {
            std::cout << "Failed to load file: " <<  as_cstring(list_get(args, 1)) << std::endl;
            return -1;
        }

        set_branch_in_progress(branch, false);

        caStack* stack = circa_alloc_stack(world);

        // Push function
        caFunction* func = circa_find_function(branch, as_cstring(list_get(args, 2)));
        circa_push_function(stack, func);

        // Push inputs
        for (int i=3, inputIndex = 0; i < circa_count(args); i++) {
            caValue* val = circa_input(stack, inputIndex++);
            circa_parse_string(as_cstring(list_get(args, i)), val);
        }

        circa_run(stack);

        if (circa_has_error(stack)) {
            circa_print_error_to_stdout(stack);
        }

        // Print outputs
        for (int i=0;; i++) {
            caValue* out = circa_output(stack, i);
            if (out == NULL)
                break;

            std::cout << to_string(circa_output(stack, i)) << std::endl;
        }
        
        circa_dealloc_stack(stack);
    }

    // Start debugger repl
    if (string_eq(list_get(args, 0), "-d"))
        return run_debugger_repl(as_cstring(list_get(args, 1)));

    // Generate cpp headers
    if (string_eq(list_get(args, 0), "-gh")) {
        Branch branch;
        load_script(&branch, as_cstring(list_get(args, 1)));

        std::cout << generate_cpp_headers(&branch);

        return 0;
    }

    // Run file checker
    if (string_eq(list_get(args, 0), "-check"))
        return run_file_checker(as_cstring(list_get(args, 1)));

    // Export parsed information
    if (string_eq(list_get(args, 0), "-export")) {
        const char* filename = "";
        const char* format = "";
        if (list_length(args) >= 2)
            format = as_cstring(list_get(args, 1));
        if (list_length(args) >= 3)
            filename = as_cstring(list_get(args, 2));
        return run_exporting_parser(format, filename);
    }

    // Build tool
    if (string_eq(list_get(args, 0), "-build")) {
        return run_build_tool(args);
    }

    // Stress test parser
    if (string_eq(list_get(args, 0), "-parse100")) {

        const char* filename = as_cstring(list_get(args, 1));

        for (int i=0; i < 100; i++) {
            Branch branch;
            load_script(&branch, filename);
            evaluate_branch(&branch);
        }
        return true;
    }
    if (string_eq(list_get(args, 0), "-parse1000")) {

        const char* filename = as_cstring(list_get(args, 1));

        for (int i=0; i < 1000; i++) {
            Branch branch;
            load_script(&branch, filename);
            evaluate_branch(&branch);
        }
        return true;
    }

    // C++ gen
    if (string_eq(list_get(args, 0), "-cppgen")) {
        Value remainingArgs;
        list_slice(args, 1, -1, &remainingArgs);
        run_generate_cpp(&remainingArgs);
        return 0;
    }

    // Command reader (from stdin)
    if (string_eq(list_get(args, 0), "-run-stdin")) {
        run_commands_from_stdin();
        return 0;
    }

    // Reproduce source text
    if (string_eq(list_get(args, 0), "-source-repro")) {
        Branch branch;
        load_script(&branch, as_cstring(list_get(args, 1)));
        std::cout << get_branch_source_text(&branch);
        return 0;
    }

    // Rewrite source, this is useful for upgrading old source
    if (string_eq(list_get(args, 0), "-rewrite-source")) {
        Branch branch;
        load_script(&branch, as_cstring(list_get(args, 1)));
        std::string contents = get_branch_source_text(&branch);
        circa_write_text_file(as_cstring(list_get(args, 1)), contents.c_str());
        return 0;
    }

    // Default behavior with no flags: load args[0] as a script and run it.
    Branch* main_branch = create_branch(kernel());
    load_script(main_branch, as_cstring(list_get(args, 0)));
    set_branch_in_progress(main_branch, false);

    if (printRawWithProps)
        print_branch_with_properties(std::cout, main_branch);
    else if (printRaw)
        print_branch(std::cout, main_branch);

    if (dontRunScript)
        return 0;

    Stack* stack = alloc_stack(NULL);

    push_frame(stack, main_branch);

    run_interpreter(stack);

    if (printState)
        std::cout << stack->state.toString() << std::endl;

    if (error_occurred(stack)) {
        std::cout << "Error occurred:\n";
        print_error_stack(stack, std::cout);
        std::cout << std::endl;
        return 1;
    }

    return 0;
}

} // namespace circa

EXPORT int circa_run_command_line(caWorld* world, int argc, const char* args[])
{
    circa::Value args_v;
    circa_set_list(&args_v, 0);
    for (int i=1; i < argc; i++)
        circa_set_string(circa_append(&args_v), args[i]);

    return circa::run_command_line(world, &args_v);
}
