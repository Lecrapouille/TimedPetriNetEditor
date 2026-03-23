//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/Receptivities.hpp"
#include "PetriNet/PetriNet.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
// Helper function to write a single net to JSON
static void writeNetToJSON(std::ofstream& file, Net const& net)
{
    std::string separator("\n");

    file << "    {" << std::endl;
    file << "       \"name\": \"" << net.name << "\"," << std::endl;

    // Places
    file << "       \"places\": [";
    for (auto const& p: net.places())
    {
        file << separator; separator = ",\n";
        file << "            { \"id\": " << p.id << ", \"caption\": \"" << p.caption
             << "\", \"tokens\": " << p.tokens << ", \"x\": " << p.x
             << ", \"y\": " << p.y << " }";
    }

    // Transitions
    separator = "\n";
    file << "\n       ],\n       \"transitions\": [";
    for (auto const& t: net.transitions())
    {
        file << separator; separator = ",\n";
        file << "            { \"id\": " << t.id << ", \"caption\": \"" << t.caption << "\", \"x\": "
             << t.x << ", \"y\": " << t.y << ", \"angle\": " << t.angle << " }";
    }

    // Arcs
    separator = "\n";
    file << "\n       ],\n       \"arcs\": [";
    for (auto const& a: net.arcs())
    {
        file << separator; separator = ",\n";
        file << "            { \"from\": \"" << a.from.key << "\", " << "\"to\": \"" << a.to.key << "\"";
        if (a.from.type == Node::Type::Transition)
            file << ", \"duration\": " << a.duration;
        file << " }";
    }

    // GRAFCET actions
    separator = "\n";
    file << "\n       ],\n       \"actions\": [";
    for (auto const& p: net.places())
    {
        for (auto const& action : p.actions)
        {
            file << separator; separator = ",\n";
            file << "            { \"place_id\": " << p.id
                 << ", \"qualifier\": \"" << qualifierToStr(action.qualifier)
                 << "\", \"color\": \"" << ledColorToStr(action.color)
                 << "\", \"name\": \"" << action.name
                 << "\", \"script\": \"" << action.script
                 << "\", \"duration\": " << action.duration << " }";
        }
    }
    file << "\n       ]" << std::endl;

    file << "    }";
}

//------------------------------------------------------------------------------
std::string exportAllNetsToJSON(std::vector<Net> const& nets, std::string const& filename)
{
    if (nets.empty())
    {
        return "No nets to export";
    }

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed saving the Petri net in '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    // Use the type of the first net for the document
    file << "{" << std::endl;
    file << "  \"revision\": 4," << std::endl;
    file << "  \"type\": \"" << to_str(nets[0].type()) << "\"," << std::endl;
    file << "  \"nets\": [" << std::endl;

    // Write all nets
    for (size_t i = 0; i < nets.size(); ++i)
    {
        writeNetToJSON(file, nets[i]);
        if (i < nets.size() - 1)
        {
            file << ",";
        }
        file << std::endl;
    }

    file << "  ]," << std::endl;

    // Inputs (shared between all nets)
    std::string separator("\n");
    file << "  \"inputs\": [";
    for (auto& it: Sensors::instance().database())
    {
        file << separator; separator = ",\n";
        file << "    { \"name\": \"" << it.first.c_str() << "\", "
             << "\"initial\": " << it.second << " }";
    }
    file << "\n  ]" << std::endl;

    file << "}" << std::endl;
    return {};
}

//------------------------------------------------------------------------------
std::string exportToJSON(Net const& net, std::string const& filename)
{
    std::string separator("\n");

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed saving the Petri net in '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    file << "{" << std::endl;
    file << "  \"revision\": 4," << std::endl;
    file << "  \"type\": \"" << to_str(net.type()) << "\"," << std::endl;
    file << "  \"nets\": [" << std::endl;

    writeNetToJSON(file, net);
    file << std::endl;

    file << "  ]," << std::endl;

    // Inputs with initial value
    separator = "\n";
    file << "  \"inputs\": [";
    for (auto& it: Sensors::instance().database())
    {
        file << separator; separator = ",\n";
        file << "    { \"name\": \"" << it.first.c_str() << "\", "
             << "\"initial\": " << it.second << " }";
    }
    file << "\n  ]" << std::endl;

    file << "}" << std::endl;
    return {};
}

} // namespace tpne
