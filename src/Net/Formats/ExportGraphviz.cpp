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
std::string exportToGraphviz(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
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
        if (t.canFire())
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
        file << "  " << a.from.key << " -> " << a.to.key;
        if (a.from.type == Node::Type::Transition)
        {
            file << " [label=\"" << a.duration << "\"]";
        }
        file << ";" << std::endl;
    }

    file << "}" << std::endl;
    return {};
}

} // namespace tpne