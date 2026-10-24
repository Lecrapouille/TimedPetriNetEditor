// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// draw.io / diagrams.net (https://www.drawio.com): an editable drawing of the
// net at the positions of the editor. Places are circles with their tokens,
// transitions rectangles; arcs show their weight, their duration and their
// color, inhibitor arcs end with a circle.

#include "IO/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief The text escaped for an XML attribute (draw.io values are HTML:
//! new lines as <br>).
static std::string xml(std::string const& text)
{
    std::string s;
    for (char const c : text)
    {
        switch (c)
        {
        case '&': s += "&amp;"; break;
        case '<': s += "&lt;"; break;
        case '>': s += "&gt;"; break;
        case '"': s += "&quot;"; break;
        case '\n': s += "&lt;br&gt;"; break;
        default: s += c; break;
        }
    }
    return s;
}

//------------------------------------------------------------------------------
std::string exportToDrawIO(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    // Sizes of the nodes, as drawn by the editor.
    float const place = 40.0f;
    float const width = 50.0f;
    float const height = 20.0f;
    bool const timed = (net.type() == TypeOfNet::TimedPetriNet);

    file << "<mxfile host=\"TimedPetriNetEditor\" type=\"device\">\n"
         << "  <diagram name=\"" << xml(net.name.empty() ? "net" : net.name) << "\" id=\"net\">\n"
         << "    <mxGraphModel grid=\"1\" gridSize=\"10\" guides=\"1\" tooltips=\"1\" connect=\"1\" arrows=\"1\""
         << " fold=\"1\" page=\"0\" math=\"0\" shadow=\"0\">\n"
         << "      <root>\n"
         << "        <mxCell id=\"0\"/>\n"
         << "        <mxCell id=\"1\" parent=\"0\"/>\n";

    // Places: the tokens in the circle, the caption under it.
    for (auto const& p : net.places())
    {
        Point const& at = net.position(p);
        // Up to 3 tokens drawn as dots, else their number.
        std::string value = (p.tokens > 3u) ? std::to_string(p.tokens) : "";
        for (size_t k = 0u; (p.tokens <= 3u) && (k < p.tokens); ++k)
            value += "\xE2\x97\x8F"; // black circle
        file << "        <mxCell id=\"" << p.key << "\" value=\"" << xml(value)
             << "\" style=\"ellipse;whiteSpace=wrap;html=1;aspect=fixed;fontSize=14;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << (at.x - place / 2.0f) << "\" y=\"" << (at.y - place / 2.0f)
             << "\" width=\"" << place << "\" height=\"" << place << "\" as=\"geometry\"/>\n"
             << "        </mxCell>\n"
             << "        <mxCell id=\"" << p.key << "_caption\" value=\"" << xml(p.caption.empty() ? p.key : p.caption)
             << "\" style=\"text;html=1;align=center;verticalAlign=top;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << (at.x - 60.0f) << "\" y=\"" << (at.y + place / 2.0f)
             << "\" width=\"120\" height=\"20\" as=\"geometry\"/>\n"
             << "        </mxCell>\n";
    }

    // Transitions: a filled rectangle, the caption under it.
    for (auto const& t : net.transitions())
    {
        Point const& at = net.position(t);
        file << "        <mxCell id=\"" << t.key << "\" value=\"\" style=\"rounded=1;whiteSpace=wrap;html=1;"
             << "fillColor=#dae8fc;strokeColor=#6c8ebf;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << (at.x - width / 2.0f) << "\" y=\"" << (at.y - height / 2.0f)
             << "\" width=\"" << width << "\" height=\"" << height << "\" as=\"geometry\"/>\n"
             << "        </mxCell>\n"
             << "        <mxCell id=\"" << t.key << "_caption\" value=\"" << xml(t.caption.empty() ? t.key : t.caption)
             << "\" style=\"text;html=1;align=center;verticalAlign=top;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << (at.x - 60.0f) << "\" y=\"" << (at.y + height / 2.0f)
             << "\" width=\"120\" height=\"20\" as=\"geometry\"/>\n"
             << "        </mxCell>\n";
    }

    // Arcs: their weight, duration and color as label.
    size_t n = 0u;
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
        std::string const head = (a.kind == ArcKind::Inhibitor) ? "endArrow=oval;endFill=0;"
                               : (a.kind == ArcKind::Reset) ? "endArrow=doubleBlock;dashed=1;" : "endArrow=classic;";
        file << "        <mxCell id=\"A" << n++ << "\" value=\"" << xml(label) << "\" style=\"" << head
             << "html=1;rounded=0;\" edge=\"1\" parent=\"1\" source=\"" << a.from.key() << "\" target=\""
             << a.to.key() << "\">\n"
             << "          <mxGeometry relative=\"1\" as=\"geometry\"/>\n"
             << "        </mxCell>\n";
    }

    file << "      </root>\n    </mxGraphModel>\n  </diagram>\n</mxfile>\n";
    return {};
}

} // namespace tpne
