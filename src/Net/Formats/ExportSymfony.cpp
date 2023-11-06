//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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
//=============================================================================

#include "Exports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string exportToSymfony(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    file << R"PN(framework:
    workflows:
)PN";
    file << "        " << net.name << ":";
    file << R"PN(
            type: 'workflow'
            audit_trail:
                enabled: true
            marking_store:
                type: 'method'
                property: 'currentPlace'
            initial_marking:
)PN";

    // Initial places
    for (auto const& p: net.places())
    {
        if (p.tokens > 0u)
        {
            file << "                - " << p.caption << std::endl;
        }
    }

    // Places
    file << "            places:" << std::endl;
    for (auto const& p: net.places())
    {
        file << "                - " << p.caption << std::endl;
    }

    // Transitions
    file << "            transitions:" << std::endl;
    for (auto const& t: net.transitions())
    {
        // From
        file << "                " << t.caption << ":" << std::endl;
        file << "                    from:" << std::endl;

        for (auto const& it: t.arcsIn)
        {
            file << "                        - " << it->from.caption << std::endl;
        }


        // To
        file << "                    to:" << std::endl;
        for (auto const& it: t.arcsOut)
        {
            file << "                        - " << it->to.caption << std::endl;
        }
    }
    return {};
}

} // namespace tpne
