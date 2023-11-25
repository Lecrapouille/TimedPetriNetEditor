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

#include "Imports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include "nlohmann/json.hpp"
#include <sstream>
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string importFromJSON(Net& net, std::string const& filename)
{
    std::stringstream error;

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason was '"
              << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    // Load the JSON content into dictionaries
    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (std::exception const& e)
    {
        error << "Failed parsing '" << filename << "'. Reason was '"
              << e.what() << "'" << std::endl;
        return error.str();
    }

    std::string type = std::string(json["type"]);
    if (type == "GRAFCET") {
        net.clear(TypeOfNet::GRAFCET);
    } else if (type == "Petri net") {
        net.clear(TypeOfNet::PetriNet);
    } else if (type == "Timed Petri net") {
        net.clear(TypeOfNet::TimedPetriNet);
    } else if (type == "Timed event graph") {
        net.clear(TypeOfNet::TimedEventGraph);
    } else {
        error << "Failed parsing '" << filename << "'. Reason was '"
              << "Unknown type of net: " << type << "'" << std::endl;
        return error.str();
    }

    nlohmann::json const& jnet = json["nets"][0];
    net.name = std::string(jnet["name"]);

    // Places
    for (nlohmann::json const& p : jnet["places"]) {
        net.addPlace(p["id"], p["caption"], p["x"], p["y"], p["tokens"]);
    }

    // Transitions
    for (nlohmann::json const& t : jnet["transitions"]) {
        net.addTransition(t["id"], t["caption"], t["x"], t["y"], t["angle"]);
    }

    // Arcs
    for (nlohmann::json const& a : jnet["arcs"])
    {
        Node* from = net.findNode(a["from"]);
        Node* to = net.findNode(a["to"]);
        if ((from == nullptr) || (to == nullptr))
        {
            error << "Failed parsing '" << filename << "'. Reason was 'Arc "
                  << a["from"] << " -> " << a["to"] << " refer to unknown nodes'"
                  << std::endl;
            return error.str();
        }

        float duration = NAN;
        auto const& it = a.find("duration");
        if (it != a.end())
        {
            duration = *it;
            if (duration < 0.0f)
            {
                error << "Failed parsing '" << filename << "'. Reason was 'Arc "
                      << from->key << " -> " << to->key << " has negative duration'"
                      << std::endl;
                return error.str();
            }
        }
        if (!net.addArc(*from, *to, duration))
        {
            error << "Failed loading " << filename
                  << ". Arc " << from->key << " -> " << to->key
                  << " is badly formed" << std::endl;
            return error.str();
        }
    }

    return {};
}

} // namespace tpne
