// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Imports.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Grafcet.hpp"
#include "PetriNet/SafeFloat.hpp"
#include "nlohmann/json.hpp"
#include <sstream>
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
// Helper function to parse a single net from JSON
static std::string parseNetFromJSON(Net& net, nlohmann::json const& jnet)
{
    std::stringstream error;

    // Net name
    if (!jnet.contains("name"))
    {
        error << "Missing JSON net name";
        return error.str();
    }
    net.name = std::string(jnet["name"]);

    // Places
    for (nlohmann::json const& p : jnet["places"]) {
        Place& place = net.addPlace(p["id"], p["caption"], p["x"], p["y"], p["tokens"]);

        // GRAFCET actions embedded in place
        if (p.contains("actions"))
        {
            for (nlohmann::json const& a : p["actions"])
            {
                Action action;
                action.qualifier = strToQualifier(a.value("qualifier", "N"));
                action.color = strToLedColor(a.value("color", "green"));
                action.name = a.value("name", "");
                action.script = a.value("script", "");
                action.duration = a.value("duration", 0.0f);
                // Parse forcings
                if (a.contains("forcing"))
                {
                    for (nlohmann::json const& f : a["forcing"])
                    {
                        action.forcings.push_back(parseForcing(f.get<std::string>()));
                    }
                }
                place.actions.push_back(action);
            }
        }
    }

    // Transitions
    for (nlohmann::json const& t : jnet["transitions"]) {
        net.addTransition(t["id"], t["caption"], t["x"], t["y"]);
    }

    // Arcs
    for (nlohmann::json const& a : jnet["arcs"])
    {
        Node* from = net.findNode(a["from"]);
        Node* to = net.findNode(a["to"]);
        if ((from == nullptr) || (to == nullptr))
        {
            error << "Arc " << a["from"] << " -> " << a["to"] << " refers to unknown nodes";
            return error.str();
        }

        float duration = safeNaNF();
        auto const& it = a.find("duration");
        if (it != a.end())
        {
            duration = *it;
            if (duration < 0.0f)
            {
                error << "Arc " << from->key << " -> " << to->key << " has negative duration";
                return error.str();
            }
        }
        auto error_msg = net.addArc(*from, *to, duration);
        if (!error_msg.empty())
        {
            error << "Arc " << from->key << " -> " << to->key << " is badly formed";
            return error.str();
        }
    }

    // GRAFCET step actions (separate array format)
    if (jnet.contains("actions"))
    {
        for (nlohmann::json const& a : jnet["actions"])
        {
            size_t place_id = a["place_id"];
            Place* place = net.findPlace(place_id);
            if (place != nullptr)
            {
                Action action;
                action.qualifier = strToQualifier(a.value("qualifier", "N"));
                action.color = strToLedColor(a.value("color", "green"));
                action.name = a.value("name", "");
                action.script = a.value("script", "");
                action.duration = a.value("duration", 0.0f);
                // Parse forcings
                if (a.contains("forcing"))
                {
                    for (nlohmann::json const& f : a["forcing"])
                    {
                        action.forcings.push_back(parseForcing(f.get<std::string>()));
                    }
                }
                place->actions.push_back(action);
            }
        }
    }

    net.resetReceptivies();
    return {};
}

//------------------------------------------------------------------------------
std::string importAllNetsFromJSON(std::vector<Net>& nets, std::string const& filename)
{
    std::stringstream error;

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '"
              << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    // Load the JSON content
    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (std::exception const& e)
    {
        error << "Failed parsing '" << filename << "'. Reason: '"
              << e.what() << "'" << std::endl;
        return error.str();
    }

    // Get the type of net
    TypeOfNet netType = TypeOfNet::PetriNet;
    if (json.contains("type"))
    {
        auto type = std::string(json["type"]);
        if (type == "GRAFCET") {
            netType = TypeOfNet::GRAFCET;
        } else if (type == "Petri net") {
            netType = TypeOfNet::PetriNet;
        } else if (type == "Timed Petri net") {
            netType = TypeOfNet::TimedPetriNet;
        } else if (type == "Timed event graph") {
            netType = TypeOfNet::TimedEventGraph;
        } else {
            error << "Failed parsing '" << filename << "'. Reason: 'Unknown type of net: " << type << "'" << std::endl;
            return error.str();
        }
    }

    if (!json.contains("nets"))
    {
        error << "Failed parsing '" << filename << "'. Reason: 'Missing JSON nets field'" << std::endl;
        return error.str();
    }

    // Parse all nets
    nets.clear();
    for (nlohmann::json const& jnet : json["nets"])
    {
        Net net(netType);
        std::string parseError = parseNetFromJSON(net, jnet);
        if (!parseError.empty())
        {
            error << "Failed parsing '" << filename << "'. Reason: '" << parseError << "'" << std::endl;
            return error.str();
        }
        nets.push_back(net);
    }

    // GRAFCET inputs (shared between all nets)
    Sensors::instance().clear();
    if (json.contains("inputs"))
    {
        for (nlohmann::json const& i : json["inputs"])
        {
            std::string name = i["name"];
            int initial = i.value("initial", 0);
            Sensors::instance().set(name, initial);
        }
    }
    // Also check in first net for backward compatibility
    else if (!nets.empty())
    {
        nlohmann::json const& jnet = json["nets"][0];
        if (jnet.contains("inputs"))
        {
            for (nlohmann::json const& i : jnet["inputs"])
            {
                std::string name = i["name"];
                int initial = i.value("initial", 0);
                Sensors::instance().set(name, initial);
            }
        }
        else if (jnet.contains("sensors"))
        {
            for (nlohmann::json const& s : jnet["sensors"])
            {
                std::string name = s["name"];
                int value = s.value("value", s.value("initial", 0));
                Sensors::instance().set(name, value);
            }
        }
    }

    return {};
}

//------------------------------------------------------------------------------
std::string importFromJSON(Net& net, std::string const& filename)
{
    // Use the new multi-net import function and take only the first net
    std::vector<Net> nets;
    std::string error = importAllNetsFromJSON(nets, filename);
    if (!error.empty())
    {
        return error;
    }

    if (nets.empty())
    {
        return "No nets found in file '" + filename + "'";
    }

    // Move first net to output
    net = nets[0];
    return {};
}

} // namespace tpne
