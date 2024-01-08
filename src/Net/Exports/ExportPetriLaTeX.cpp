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

#include "Net/Exports/Exports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <fstream>
#include <cstring>
#include <iomanip>

namespace tpne {

// FIXME
static float const scale_x = 1.0f;
static float const scale_y = 1.0f;

//------------------------------------------------------------------------------
std::string exportToPetriLaTeX(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    file << R"PN(\documentclass[border = 0.2cm]{standalone}
\usepackage{tikz}
\usetikzlibrary{petri,positioning}
\begin{document}
\begin{tikzpicture}
)PN";

    // Places
    file << std::endl << "% Places" << std::endl;
    for (auto const& p: net.places())
    {
        file << "\\node[place, "
             << "label=above:$" << p.caption << "$, "
             << "fill=blue!25, "
             << "draw=blue!75, "
             << "tokens=" << p.tokens << "] "
             << "(" << p.key << ") at (" << int(p.x * scale_x)
             << ", " << int(-p.y * scale_y) << ") {};"
             << std::endl;
    }

    // Transitions
    file << std::endl << "% Transitions" << std::endl;
    for (auto const& t: net.transitions())
    {
        std::string color = (t.isFireable() ? "green" : "red");

        file << "\\node[transition, "
             << "label=above:$" << t.caption << "$, "
             << "fill=" << color << "!25, "
             << "draw=" << color << "!75] "
             << "(" << t.key << ") at (" << int(t.x * scale_x)
             << ", " << int(-t.y * scale_y) << ") {};"
             << std::endl;
    }

    // Arcs
    file << std::endl << "% Arcs" << std::endl;
    for (auto const& a: net.arcs())
    {
        if (a.from.type == Node::Type::Transition)
        {
            std::stringstream duration;
            duration << std::fixed << std::setprecision(2) << a.duration;
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- "
                 << "node[midway, above right] "
                 << "{" << duration.str() << "} "
                 << "(" << a.to.key << ");"
                 << std::endl;
        }
        else
        {
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- " << "(" << a.to.key << ");"
                 << std::endl;
        }
    }

    file << R"PN(
\end{tikzpicture}
\end{document}
)PN";

    return {};
}

} // namespace tpne
