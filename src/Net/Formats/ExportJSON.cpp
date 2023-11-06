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
#include "nlohmann/json.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string exportToJSON(Net const& net, std::string const& filename)
{
    std::string separator("\n");

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed saving the Petri net in '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    // TODO sensors

    file << "{" << std::endl;
    file << "  \"revision\": 3," << std::endl;
    file << "  \"type\": \"" << to_str(net.type()) << "\"," << std::endl;
    file << "  \"nets\": [\n    {" << std::endl;
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
        file << "            { \"from\": \"" << a.from.key << "\", " << "\"to\": \"" << a.to.key
             << "\"";
        if (a.from.type == Node::Type::Transition)
            file << ", \"duration\": " << a.duration;
        file << " }";
    }
    file << "\n       ]" << std::endl;
    file << "    }" << std::endl;
    file << "  ]" << std::endl;
    file << "}" << std::endl;

    return {};
}

} // namespace tpne
