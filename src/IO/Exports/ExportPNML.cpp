// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Petri Net Markup Language (https://www.pnml.org), place/transition nets:
// places with their initial marking, transitions, arcs with their weight
// (inscription). What PNML does not know (type of net, durations, kinds of
// arcs, capacities, priorities, delays) is in <toolspecific> elements that
// other tools ignore. Symmetri (https://github.com/thorstink/Symmetri) reads
// its own flavor: ids are the names of the nodes, one <initialMarking> per
// token, arcs with a color attribute.

#include "IO/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

namespace tpne {

//! \brief Name of this tool in the <toolspecific> elements.
static char const* const TOOL = "TimedPetriNetEditor";

//------------------------------------------------------------------------------
//! \brief The text escaped for XML.
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
        case '\'': s += "&apos;"; break;
        default: s += c; break;
        }
    }
    return s;
}

//------------------------------------------------------------------------------
//! \brief PNML identifiers of the nodes: their caption (Symmetri calls the
//! functions by them) when unique among all the nodes, else their key.
static std::map<NodeId, std::string, bool(*)(NodeId const&, NodeId const&)>
identifiers(Net const& net)
{
    std::map<std::string, size_t> count;
    auto name = [](Node const& n) { return n.caption.empty() ? n.key : n.caption; };
    for (auto const& p : net.places())
        ++count[name(p)];
    for (auto const& t : net.transitions())
        ++count[name(t)];
    std::map<NodeId, std::string, bool(*)(NodeId const&, NodeId const&)> ids(
        [](NodeId const& a, NodeId const& b) {
            return (a.type != b.type) ? (a.type < b.type) : (a.id < b.id);
        });
    auto add = [&](Node const& n) {
        std::string const s = name(n);
        ids[n.nodeId()] = ((count[s] == 1u) || (s == n.key)) ? s : n.key;
    };
    for (auto const& p : net.places())
        add(p);
    for (auto const& t : net.transitions())
        add(t);
    return ids;
}

//------------------------------------------------------------------------------
static std::string writePNML(Net const& net, std::string const& filename, bool const symmetri)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    auto const ids = identifiers(net);
    std::string const tool = std::string("<toolspecific tool=\"") + TOOL + "\" version=\"1\">";

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<pnml xmlns=\"http://www.pnml.org/version-2009/grammar/pnml\">\n"
         << "  <net id=\"" << xml(net.name.empty() ? "net" : net.name)
         << "\" type=\"http://www.pnml.org/version-2009/grammar/ptnet\">\n"
         << "    <name><text>" << xml(net.name) << "</text></name>\n"
         << "    " << tool << "<type>" << xml(to_str(net.type())) << "</type>"
         << (net.layout().compact ? "<compact>true</compact>" : "") << "</toolspecific>\n"
         << "    <page id=\"page0\">\n";

    for (auto const& p : net.places())
    {
        Point const& at = net.position(p);
        file << "      <place id=\"" << xml(ids.at(p.nodeId())) << "\">\n"
             << "        <name><text>" << xml(p.caption) << "</text></name>\n"
             << "        <graphics><position x=\"" << at.x << "\" y=\"" << at.y << "\"/></graphics>\n";
        if (symmetri)
        {
            // One element per token.
            for (size_t k = 0u; k < p.tokens; ++k)
                file << "        <initialMarking><text>1</text></initialMarking>\n";
        }
        else if (p.tokens > 0u)
        {
            file << "        <initialMarking><text>" << p.tokens << "</text></initialMarking>\n";
        }
        if ((p.capacity != std::numeric_limits<size_t>::max()) &&
            (p.capacity != Net::defaultCapacity(net.type())))
            file << "        " << tool << "<capacity>" << p.capacity << "</capacity></toolspecific>\n";
        file << "      </place>\n";
    }

    for (auto const& t : net.transitions())
    {
        Point const& at = net.position(t);
        file << "      <transition id=\"" << xml(ids.at(t.nodeId())) << "\">\n"
             << "        <name><text>" << xml(t.caption) << "</text></name>\n"
             << "        <graphics><position x=\"" << at.x << "\" y=\"" << at.y << "\"/></graphics>\n";
        if ((t.priority != 0) || (t.delay > 0.0f))
        {
            file << "        " << tool;
            if (t.priority != 0)
                file << "<priority>" << t.priority << "</priority>";
            if (t.delay > 0.0f)
                file << "<delay>" << t.delay << "</delay>";
            file << "</toolspecific>\n";
        }
        file << "      </transition>\n";
    }

    size_t n = 0u;
    for (auto const& a : net.arcs())
    {
        file << "      <arc id=\"A" << n++ << "\" source=\"" << xml(ids.at(a.from))
             << "\" target=\"" << xml(ids.at(a.to)) << "\"";
        if (a.color != AnyColor)
            file << " color=\"" << xml(net.colors.name(a.color)) << "\"";
        file << ">\n";
        if (a.weight != 1u)
            file << "        <inscription><text>" << a.weight << "</text></inscription>\n";
        bool const timed = a.from.isTransition() && (a.durationOrZero() != 0.0f);
        if (timed || (a.kind != ArcKind::Normal))
        {
            file << "        " << tool;
            if (timed)
                file << "<duration>" << *a.duration << "</duration>";
            if (a.kind != ArcKind::Normal)
                file << "<kind>" << to_str(a.kind) << "</kind>";
            file << "</toolspecific>\n";
        }
        file << "      </arc>\n";
    }

    file << "    </page>\n  </net>\n</pnml>\n";
    return {};
}

//------------------------------------------------------------------------------
std::string exportToPNML(Net const& net, std::string const& filename)
{
    return writePNML(net, filename, false);
}

//------------------------------------------------------------------------------
std::string exportToSymmetriPNML(Net const& net, std::string const& filename)
{
    return writePNML(net, filename, true);
}

} // namespace tpne
