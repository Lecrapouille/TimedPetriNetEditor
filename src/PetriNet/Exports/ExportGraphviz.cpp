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
std::string exportToGraphviz(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    file << "digraph G {" << std::endl;

    // Places
    file << "node [shape=circle, color=blue]" << std::endl;
    for (auto const& p: net.places())
    {
        file << "  " << p.key << " [label=\"" << p.caption;
        if (p.tokens > 0u)
        {
            file << "\\n" << p.tokens << "&bull;";
        }
        file << "\"];" << std::endl;
    }

    // Transitions
    file << "node [shape=box, color=red]" << std::endl;
    for (auto const& t: net.transitions())
    {
        if (net.canFire(t))
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\", color=green];"
                 << std::endl;
        }
        else
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\"];"
                 << std::endl;
        }
    }

    // Arcs
    file << "edge [style=\"\"]" << std::endl;
    for (auto const& a: net.arcs())
    {
        file << "  " << a.from.key() << " -> " << a.to.key();
        if (a.from.type == Node::Type::Transition)
        {
            file << " [label=\"" << a.durationOrZero() << "\"]";
        }
        file << ";" << std::endl;
    }

    file << "}" << std::endl;
    return {};
}

} // namespace tpne
