// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Graphviz (https://graphviz.org): dot -Tsvg net.dot -o net.svg. Places are
// circles with their tokens, transitions boxes; the arcs show their weight,
// their duration (timed nets), their color (runtime) and their kind
// (inhibitor: a circle head, reset: a double head).

#include "IO/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief The text as a quoted dot string.
static std::string quote(std::string const& text)
{
    std::string s = "\"";
    for (char const c : text)
    {
        if ((c == '"') || (c == '\\'))
            s += '\\';
        if (c == '\n')
            s += "\\n";
        else
            s += c;
    }
    return s + "\"";
}

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

    bool const timed = (net.type() == TypeOfNet::TimedPetriNet);
    file << "digraph " << quote(net.name.empty() ? "net" : net.name) << " {\n"
         << "  rankdir=LR;\n"
         << "  node [fontname=\"Helvetica\"];\n"
         << "  edge [fontname=\"Helvetica\", fontsize=10];\n";

    // Places: their caption and their tokens (bullets up to 5).
    file << "  node [shape=circle, fixedsize=false];\n";
    for (auto const& p : net.places())
    {
        std::string label = p.caption.empty() ? p.key : p.caption;
        if (p.tokens > 0u)
        {
            label += "\n";
            if (p.tokens <= 5u)
                for (size_t k = 0u; k < p.tokens; ++k)
                    label += "\xE2\x80\xA2"; // bullet
            else
                label += std::to_string(p.tokens);
        }
        file << "  " << p.key << " [label=" << quote(label);
        if (p.capacity != std::numeric_limits<size_t>::max() && (p.capacity != Net::defaultCapacity(net.type())))
            file << ", xlabel=" << quote("max " + std::to_string(p.capacity));
        file << "];\n";
    }

    // Transitions: their caption (receptivity of a GRAFCET).
    file << "  node [shape=box, height=0.3];\n";
    for (auto const& t : net.transitions())
        file << "  " << t.key << " [label=" << quote(t.caption.empty() ? t.key : t.caption) << "];\n";

    for (auto const& a : net.arcs())
    {
        std::string label;
        if (a.weight != 1u)
            label = std::to_string(a.weight);
        if (timed && a.from.isTransition() && (a.durationOrZero() != 0.0f))
        {
            std::ostringstream d;
            d << a.durationOrZero();
            label += (label.empty() ? "" : ", ") + d.str() + " s";
        }
        if (a.color != AnyColor)
            label += (label.empty() ? "" : ", ") + net.colors.name(a.color);
        file << "  " << a.from.key() << " -> " << a.to.key() << " [";
        std::string sep;
        if (!label.empty())
        {
            file << "label=" << quote(label);
            sep = ", ";
        }
        if (a.kind == ArcKind::Inhibitor)
            file << sep << "arrowhead=odot";
        else if (a.kind == ArcKind::Reset)
            file << sep << "arrowhead=normalnormal, style=dashed";
        file << "];\n";
    }
    file << "}\n";
    return {};
}

} // namespace tpne
