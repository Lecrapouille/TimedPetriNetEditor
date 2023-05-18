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
      << name << " [petri.json]" << std::endl
      << "Where:" << std::endl
      << "  [petri.json] is an optional Petri net file to load (i.e. examples/Howard1.json)" << std::endl
      << std::endl;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse the command line
    const char* filename = nullptr;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argv[0]);
                std::cout << PetriEditor::help().str() << std::endl;
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

    Application application(1200, 720, "Timed Petri Net Editor");
    try
    {
        PetriNet net(PetriNet::Type::TimedPetri);
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
