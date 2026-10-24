// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp" // Selected by Makefile
#include <cstdlib>
#include <iostream>
#include <unistd.h> // getopt

//------------------------------------------------------------------------------
static void usage(const char* name)
{
    std::cout
      << name << " [-r] [-s|-w screenshot.png] [-d seconds] [-f window] [-c endpoint] [petri.json]" << std::endl
      << "Where:" << std::endl
      << "  [petri.json] is an optional Petri net file to load (i.e. examples/Howard1.json)" << std::endl
      << "  -r starts the simulation once the file is loaded" << std::endl
      << "  -s screenshot.png saves the net as a PNG file once drawn, then quits" << std::endl
      << "  -w screenshot.png saves the whole window as a PNG file once drawn, then quits" << std::endl
      << "  -d seconds waits before the screenshot (the simulation runs meanwhile)" << std::endl
      << "  -f window brings this window in front (Journal, Console, Analysis or one of its tabs: Critical circuit, Gantt ...)" << std::endl
      << "  -c endpoint follows the program publishing its journal there (tcp://localhost:5556)" << std::endl
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

    bool whole_window = false;
    bool start = false;
    double delay = 0.0;
    std::string focus;
    std::string endpoint;
    while ((opt = getopt(argc, argv, "hrs:w:d:f:c:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            case 's':
                screenshot = optarg;
                break;
            case 'w':
                screenshot = optarg;
                whole_window = true;
                break;
            case 'r':
                start = true;
                break;
            case 'd':
                delay = std::atof(optarg);
                break;
            case 'f':
                focus = optarg;
                break;
            case 'c':
                endpoint = optarg;
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
        editor.setAutoScreenshot(screenshot, true, whole_window);
    }
    editor.screenshotDelay(delay);
    if (!focus.empty())
    {
        editor.focusWindowAtLaunch(focus);
    }
    if (!endpoint.empty())
    {
        editor.connectAtLaunch(endpoint);
    }
    if (start)
    {
        editor.startSimulationAtLaunch();
    }
    editor.run(filename);

    if (!screenshot.empty() && !editor.lastScreenshotSucceeded())
    {
        std::cerr << "Failed saving the screenshot " << screenshot << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
