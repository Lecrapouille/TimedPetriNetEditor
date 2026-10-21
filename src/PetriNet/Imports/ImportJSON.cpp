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
#include <map>
#include <vector>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Files before revision 5 wrote GRAFCET receptivities in reverse
//! polish notation ("a b . !"). Return the infix form ("/(a . b)"), or the
//! code itself if it is not in this notation.
static std::string rpnToInfix(std::string const& code)
{
    std::vector<std::string> tokens;
    std::istringstream stream(code);
    std::string token;
    while (stream >> token)
        tokens.push_back(token);
    if (tokens.size() < 2u)
        return code;

    // Operand stack: text and whether it is a compound expression.
    std::vector<std::pair<std::string, bool>> stack;
    for (auto const& t : tokens)
    {
        if ((t == ".") || (t == "+"))
        {
            if (stack.size() < 2u)
                return code;
            auto right = stack.back(); stack.pop_back();
            auto left = stack.back(); stack.pop_back();
            auto const wrap = [&t](std::pair<std::string, bool> const& e) {
                // "a + b" inside an and needs parentheses.
                return (e.second && (t == ".")) ? "(" + e.first + ")" : e.first;
            };
            stack.push_back({ wrap(left) + " " + t + " " + wrap(right), t == "+" });
        }
        else if (t == "!")
        {
            if (stack.empty())
                return code;
            auto operand = stack.back(); stack.pop_back();
            bool const simple = (operand.first.find(' ') == std::string::npos);
            stack.push_back({ "/" + (simple ? operand.first : "(" + operand.first + ")"), false });
        }
        else
        {
            stack.push_back({ t, false });
        }
    }
    return (stack.size() == 1u) ? stack.back().first : code;
}

//------------------------------------------------------------------------------
//! \brief Read a GRAFCET action.
static Action parseAction(nlohmann::json const& a)
{
    Action action;
    action.qualifier = strToQualifier(a.value("qualifier", "N"));
    action.color = strToLedColor(a.value("color", "green"));
    action.name = a.value("name", "");
    action.script = a.value("script", "");
    action.duration = a.value("duration", 0.0f);
    action.condition = a.value("condition", "");
    if (a.contains("forcing"))
    {
        for (nlohmann::json const& f : a["forcing"])
        {
            action.forcings.push_back(parseForcing(f.get<std::string>()));
        }
    }
    return action;
}

//------------------------------------------------------------------------------
//! \brief Read GRAFCET inputs: [{ "name": "a", "initial": 0 }, ...]. Old files
//! used "sensors" with "value".
static void parseInputs(nlohmann::json const& jinputs, std::map<std::string, int>& inputs)
{
    for (nlohmann::json const& i : jinputs)
    {
        std::string const name = i["name"];
        inputs[name] = i.value("initial", i.value("value", 0));
    }
}

//------------------------------------------------------------------------------
// Helper function to parse a single net from JSON
static std::string parseNetFromJSON(Net& net, nlohmann::json const& jnet, int const revision)
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
                place.actions.push_back(parseAction(a));
            }
        }
    }

    // Transitions. GRAFCET receptivities were in reverse polish notation
    // before revision 5.
    bool const old_grafcet = (net.type() == TypeOfNet::GRAFCET) && (revision < 5);
    for (nlohmann::json const& t : jnet["transitions"]) {
        std::string caption = t["caption"];
        if (old_grafcet)
            caption = rpnToInfix(caption);
        Transition& transition = net.addTransition(t["id"], caption, t["x"], t["y"]);
        transition.delay = t.value("delay", 0.0f);
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
                place->actions.push_back(parseAction(a));
            }
        }
    }

    // GRAFCET inputs of this net
    if (jnet.contains("inputs"))
        parseInputs(jnet["inputs"], net.inputs);
    else if (jnet.contains("sensors"))
        parseInputs(jnet["sensors"], net.inputs);

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
    int const revision = json.value("revision", 0);
    nets.clear();
    for (nlohmann::json const& jnet : json["nets"])
    {
        Net net(netType);
        std::string parseError = parseNetFromJSON(net, jnet, revision);
        if (!parseError.empty())
        {
            error << "Failed parsing '" << filename << "'. Reason: '" << parseError << "'" << std::endl;
            return error.str();
        }
        nets.push_back(net);
    }

    // GRAFCET inputs shared between all nets of the document.
    if (json.contains("inputs"))
    {
        std::map<std::string, int> inputs;
        parseInputs(json["inputs"], inputs);
        for (auto& net : nets)
            net.inputs.insert(inputs.begin(), inputs.end());
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
