// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// GRML (CosyVerif): the format of GreatSPN, also read by Symmetri. See
// ImportGRML.cpp. Transitions are IMMEDIATE, or DETERMINISTIC with the
// duration of their output arcs in a timed net.

#include "IO/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
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
        default: s += c; break;
        }
    }
    return s;
}

//------------------------------------------------------------------------------
std::string exportToGRML(Net const& net, std::string const& filename)
{
    if (features(net) & Feature::Resets)
    {
        return "Failed to export the Petri net to '" + filename +
               "'. Reason: 'GRML has no reset arcs'\n";
    }
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    // Identifiers: the places, then the transitions, then the arcs.
    size_t const places = net.places().size();
    auto id = [places](NodeId const n) { return 1u + (n.isPlace() ? n.id : places + n.id); };
    auto name = [](Node const& n) { return xml(n.caption.empty() ? n.key : n.caption); };

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<model id=\"0\" formalismUrl=\"http://formalisms.cosyverif.org/sptgd-net.fml\""
         << " xmlns=\"http://cosyverif.org/ns/model\">\n";
    for (auto const& p : net.places())
    {
        Point const& at = net.position(p);
        file << "<node id=\"" << id(p.nodeId()) << "\" nodeType=\"place\" x=\"" << at.x << "\" y=\"" << at.y << "\">\n"
             << "  <attribute name=\"name\">" << name(p) << "</attribute>\n"
             << "  <attribute name=\"marking\"><attribute name=\"expr\"><attribute name=\"intValue\">"
             << p.tokens << "</attribute></attribute></attribute>\n"
             << "</node>\n";
    }
    bool const timed = (net.type() == TypeOfNet::TimedPetriNet);
    for (auto const& t : net.transitions())
    {
        Point const& at = net.position(t);
        float delay = 0.0f;
        for (auto const& a : net.arcsOut(t))
            delay = std::max(delay, a.durationOrZero());
        file << "<node id=\"" << id(t.nodeId()) << "\" nodeType=\"transition\" x=\"" << at.x << "\" y=\"" << at.y << "\">\n"
             << "  <attribute name=\"name\">" << name(t) << "</attribute>\n";
        if (timed && (delay > 0.0f))
        {
            file << "  <attribute name=\"distribution\"><attribute name=\"type\">DETERMINISTIC</attribute>"
                 << "<attribute name=\"param\"><attribute name=\"expr\"><attribute name=\"numValue\">"
                 << delay << "</attribute></attribute></attribute></attribute>\n";
        }
        else
        {
            file << "  <attribute name=\"distribution\"><attribute name=\"type\">IMMEDIATE</attribute></attribute>\n"
                 << "  <attribute name=\"weight\"><attribute name=\"expr\"><attribute name=\"numValue\">1.0"
                 << "</attribute></attribute></attribute>\n";
        }
        file << "  <attribute name=\"priority\"><attribute name=\"expr\"><attribute name=\"intValue\">"
             << t.priority << "</attribute></attribute></attribute>\n"
             << "</node>\n";
    }
    size_t n = 1u + places + net.transitions().size();
    for (auto const& a : net.arcs())
    {
        file << "<arc id=\"" << n++ << "\" arcType=\"" << ((a.kind == ArcKind::Inhibitor) ? "inhibitorarc" : "arc")
             << "\" source=\"" << id(a.from) << "\" target=\"" << id(a.to) << "\"";
        if (a.color != AnyColor)
            file << " color=\"" << xml(net.colors.name(a.color)) << "\"";
        file << ">\n  <attribute name=\"valuation\"><attribute name=\"expr\"><attribute name=\"intValue\">"
             << a.weight << "</attribute></attribute></attribute>\n</arc>\n";
    }
    file << "</model>\n";
    return {};
}

} // namespace tpne
