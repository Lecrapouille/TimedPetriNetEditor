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

namespace tpne {

//------------------------------------------------------------------------------
std::string exportToPNML(Net const& net, std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
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