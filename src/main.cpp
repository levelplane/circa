// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "circa.h"

int main(int argc, const char * args[])
{
    std::vector<std::string> argv;

    for (int i = 1; i < argc; i++)
        argv.push_back(args[i]);

    circa::initialize();

    int result = circa::run_command_line(argv);

    circa::shutdown();
    
    return result;
}
