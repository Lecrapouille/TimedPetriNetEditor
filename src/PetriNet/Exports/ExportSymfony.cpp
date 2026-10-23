// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"
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
              << "'. Reason: " << strerror(errno) << std::endl;
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

        for (auto const& it: net.arcsIn(t))
        {
            file << "                        - " << net.node(it.from).caption << std::endl;
        }


        // To
        file << "                    to:" << std::endl;
        for (auto const& it: net.arcsOut(t))
        {
            file << "                        - " << net.node(it.to).caption << std::endl;
        }
    }
    return {};
}

} // namespace tpne
