#include "Editor/Editor.hpp" // Selected by Makefile
#include <iostream>
#include <unistd.h> // getopt

//------------------------------------------------------------------------------
static void usage(const char* name)
{
    std::cout
      << name << " [-r] [-s|-w screenshot.png] [petri.json]" << std::endl
      << "Where:" << std::endl
      << "  [petri.json] is an optional Petri net file to load (i.e. examples/Howard1.json)" << std::endl
      << "  -r starts the simulation once the file is loaded" << std::endl
      << "  -s screenshot.png saves the net as a PNG file once drawn, then quits" << std::endl
      << "  -w screenshot.png saves the whole window as a PNG file once drawn, then quits" << std::endl
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
    while ((opt = getopt(argc, argv, "hrs:w:")) != -1)
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
