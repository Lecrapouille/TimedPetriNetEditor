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

//------------------------------------------------------------------------------
static void usage(const char* name)
{
    std::cout
      << name << " [petri.json]" << std::endl
      << "Left mouse button pressed: add a place" << std::endl
      << "Right mouse button pressed: add a transition" << std::endl
      << "Middle mouse button pressed: add an arc with the selected place or transition as origin" << std::endl
      << "Middle mouse button release: end the arc with the selected place or transition as destination" << std::endl
      << "L key: add an arc with the selected place or transition as origin" << std::endl
      << "Delete key: remove a place or transition or an arc" << std::endl
      << "Z key: clear the whole Petri net" << std::endl
      << "M key: move the selected place or transition" << std::endl
      << "+ key: add a token on the place pointed by the mouse cursor" << std::endl
      << "- key: remove a token on the place pointed by the mouse cursor" << std::endl
      << "R key: run (start) or stop the simulation" << std::endl
      << "SPACE key: run (start) or stop the simulation" << std::endl
      << "C key: show critical circuit" << std::endl
      << "S key: save the Petri net to petri.json file" << std::endl
      << "O key: load the Petri net from petri.json file" << std::endl
      << "G key: export the Petri net as Grafcet in a C++ header file" << std::endl
      << "J key: export the Petri net as Julia code" << std::endl
      << std::endl;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    usage(argv[0]);

    PetriNet net;
    if (argc > 2)
    {
        std::cerr << argv[0] << ": Failed needs zero or one parameter" << std::endl;
        return EXIT_FAILURE;
    }
    else if ((argc == 2) && (argv[1][0] != '-'))
    {
        if (!net.load(argv[1])) // FIXME: editor.m_file = argv[1]
        {
            std::cerr << argv[0] << ": Failed loading file '" << argv[1] << "'"
                      << std::endl;
            //return EXIT_FAILURE;
        }
    }

    Application application(800, 600, "Timed Petri Net Editor");
    PetriEditor editor(application.renderer(), net);
    editor.bgColor = sf::Color(255,255,255,255);

    try
    {
        application.push(editor);
        application.loop();
    }
    catch (std::string const& msg)
    {
        std::cerr << argv[0] << ": Fatal: " << msg << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
