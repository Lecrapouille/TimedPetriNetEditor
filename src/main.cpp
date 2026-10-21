// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp" // Selected by Makefile
#include <iostream>
#include <unistd.h> // getopt

//------------------------------------------------------------------------------
static void usage(const char* name)
{
    std::cout
      << name << " [-s screenshot.png] [petri.json]" << std::endl
      << "Where:" << std::endl
      << "  [petri.json] is an optional Petri net file to load (i.e. examples/Howard1.json)" << std::endl
      << "  -s screenshot.png saves the window as a PNG file once drawn, then quits" << std::endl
      << std::endl;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse the command line
    std::string filename;
    std::string screenshot;
    opterr = 0;
    int opt;

    while ((opt = getopt(argc, argv, "hs:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            case 's':
                screenshot = optarg;
                break;
            case '?':
                std::cerr << "Unknown option: '" << char(optopt) << "'!" << std::endl;
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Parse optional arguments
    for (; optind < argc; optind++)
    {
        filename = argv[optind];
    }

    tpne::Editor editor(1100, 768, "Petri Net Editor");
    if (!screenshot.empty())
    {
        editor.setAutoScreenshot(screenshot, true);
    }
    editor.run(filename);

    if (!screenshot.empty() && !editor.lastScreenshotSucceeded())
    {
        std::cerr << "Failed saving the screenshot " << screenshot << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
