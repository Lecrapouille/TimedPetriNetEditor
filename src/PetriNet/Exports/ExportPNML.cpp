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
std::string exportToPNML(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    file << R"PN(<?xml version="1.0" encoding="UTF-8"?>)PN" << std::endl;
    file << "<pnml>" << std::endl;
    file << "    <net id=\"" << to_str(net.type()) << "\" type=\"http://www.pnml.org/version-2009/grammar/ptnet\">" << std::endl;
    file << "       <name><text>" << net.name << "</text></name>" << std::endl;
    file << "       <page id=\"1\">" << std::endl;

    // Places
    for (auto const& p: net.places())
    {
        file << "       <place id=\"" << p.key << "\">" << std::endl;
        file << "           <name><text>" << p.caption << "</text>" << std::endl;
        file << "           <graphics><offset x=\"0\" y=\"0\"/></graphics></name>" << std::endl;
        file << "           <graphics><position x=\"" << p.x << "\" y=\"" << p.y << "\"/></graphics>" << std::endl;
        file << "           <initialMarking><text>" << p.tokens << "</text></initialMarking>" << std::endl;
        file << "       </place>" << std::endl;
    }

    // Transitions
    for (auto const& t: net.transitions())
    {
        file << "       <transition id=\"" << t.key << "\">" << std::endl;
        file << "           <name><text>" << t.caption << "</text><graphics><offset x=\"0\" y=\"0\"/></graphics></name>" << std::endl;
        file << "           <graphics><position x=\"" << t.x << "\" y=\"" << t.y << "\"/></graphics>" << std::endl;
        file << "       </transition>" << std::endl;
    }

    // Arcs
    for (auto const& a: net.arcs())
    {
        file << "       <arc id=\"" << a.from.key << a.to.key << "\" source=\"" << a.from.key << "\" target=\"" << a.to.key << "\">" << std::endl;
        file << "           <inscription><text>" << a.duration << "</text></inscription>" << std::endl;
        file << "           <graphics/>" << std::endl;
        file << "       </arc>" << std::endl;
    }

    file << "</page></net></pnml>" << std::endl;
    return {};
}

} // namespace tpne