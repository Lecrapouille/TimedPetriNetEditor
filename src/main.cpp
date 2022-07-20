//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "PetriEditor.hpp"
#include <iostream>
#include <unistd.h>

//------------------------------------------------------------------------------
static void usage(const char* name)
{
    std::cout
      << name << " [-t|-p|-g] [petri.json]" << std::endl
      << "Where:" << std::endl
      << "  [-t|-p|-g] optional argument to force the type of net:" << std::endl
      << "    -t for using a timed petri mode (by default)" << std::endl
      << "    -p for using a petri mode" << std::endl
      << "    -g for using a GRAFCET mode" << std::endl
      << "  [petri.json] is an optional Petri net file to load (i.e. examples/Howard1.json)" << std::endl
      << std::endl;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse the command line
    const char* filename = nullptr;
    PetriNet::Type type = PetriNet::Type::TimedPetri;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "gtph")) != -1)
    {
        switch (opt)
        {
            case 'g':
                type = PetriNet::Type::GRAFCET;
                std::cout << "GRAFCET mode" << std::endl;
                break;
            case 't':
                type = PetriNet::Type::TimedPetri;
                std::cout << "Timed Petri mode" << std::endl;
                break;
            case 'p':
                type = PetriNet::Type::Petri;
                std::cout << "Petri mode" << std::endl;
                break;
            case 'h':
                usage(argv[0]);
                std::cout << PetriNet::help().str() << std::endl;
                return EXIT_FAILURE;
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

    std::cout << PetriNet::help().str() << std::endl;
    Application application(800, 600, "Timed Petri Net Editor");
    try
    {
        PetriNet net(type);
        if (filename != nullptr)
        {
            PetriEditor editor(application, net, filename);
            application.loop(editor);
        }
        else
        {
            PetriEditor editor(application, net);
            application.loop(editor);
        }
    }
    catch (std::string const& msg)
    {
        std::cerr << argv[0] << ": Fatal: " << msg << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
