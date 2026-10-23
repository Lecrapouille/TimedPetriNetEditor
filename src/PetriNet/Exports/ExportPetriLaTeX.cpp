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
              << "'. Reason: " << strerror(errno) << std::endl;
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
        std::string color = (net.canFire(t) ? "green" : "red");

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
                 << "(" << a.from.key() << ") -- "
                 << "node[midway, above right] "
                 << "{" << duration.str() << "} "
                 << "(" << a.to.key() << ");"
                 << std::endl;
        }
        else
        {
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key() << ") -- " << "(" << a.to.key() << ");"
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
