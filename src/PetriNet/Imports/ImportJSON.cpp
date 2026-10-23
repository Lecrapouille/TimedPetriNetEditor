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
//! \brief Old files wrote GRAFCET receptivities in reverse polish notation
//! ("a b . !"). Return the infix form ("/(a . b)"), or the code itself if it
//! is not in this notation.
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
//! \brief Read GRAFCET inputs: [{ "name": "a", "kind": "button", "initial": 0,
//! "description": "..." }, ...]. Old files used "sensors" with "value".
//! The initial values go to `inputs`, the whole descriptions to `described`
//! when it is not null.
static void parseInputs(nlohmann::json const& jinputs, std::map<std::string, int>& inputs,
                        std::vector<GrafcetInput>* described)
{
    for (nlohmann::json const& i : jinputs)
    {
        GrafcetInput input;
        input.name = i["name"];
        input.initial = i.value("initial", i.value("value", 0));
        inputs[input.name] = input.initial;
        if (described == nullptr)
            continue;
        input.kind = strToInputKind(i.value("kind", "switch"));
        input.description = i.value("description", "");
        input.group = i.value("group", "");
        input.color = i.value("color", "");
        input.min = i.value("min", 0);
        input.max = i.value("max", 100);
        input.unit = i.value("unit", "");
        input.driven_by = i.value("driven_by", "");
        input.rise = i.value("rise", 0.0f);
        input.fall = i.value("fall", 0.0f);
        input.period = i.value("period", 1.0f);
        described->push_back(input);
    }
}

//------------------------------------------------------------------------------
//! \brief Read GRAFCET outputs: [{ "name": "Motor", "kind": "motor",
//! "description": "..." }, ...].
static void parseOutputs(nlohmann::json const& joutputs, std::vector<GrafcetOutput>& outputs)
{
    for (nlohmann::json const& o : joutputs)
    {
        GrafcetOutput output;
        output.name = o["name"];
        output.kind = strToOutputKind(o.value("kind", "light"));
        output.description = o.value("description", "");
        outputs.push_back(output);
    }
}

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
    net.description = jnet.value("description", "");

    // Identifiers of nodes are their indices in the net: they shall be
    // 0 .. n-1, in any order in the file.
    auto const sorted = [&error](nlohmann::json const& nodes, char const* what)
    {
        std::vector<nlohmann::json const*> result(nodes.size(), nullptr);
        for (nlohmann::json const& n : nodes)
        {
            size_t const id = n["id"];
            if ((id >= result.size()) || (result[id] != nullptr))
            {
                error << "The identifiers of " << what << " shall be 0 .. "
                      << (result.size() - 1u) << " without duplicate (" << id << ")";
                return std::vector<nlohmann::json const*>{};
            }
            result[id] = &n;
        }
        return result;
    };
    auto const places = sorted(jnet["places"], "places");
    if (!error.str().empty())
        return error.str();
    auto const transitions = sorted(jnet["transitions"], "transitions");
    if (!error.str().empty())
        return error.str();

    // Places
    for (nlohmann::json const* jp : places) {
        nlohmann::json const& p = *jp;
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

    // Transitions. Old files wrote GRAFCET receptivities in reverse polish
    // notation: converted when they are not a valid infix expression.
    bool const grafcet = (net.type() == TypeOfNet::GRAFCET);
    for (nlohmann::json const* jt : transitions) {
        nlohmann::json const& t = *jt;
        std::string caption = t["caption"];
        if (grafcet && !Receptivity::check(caption).empty())
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
        parseInputs(jnet["inputs"], net.inputs, nullptr);
    else if (jnet.contains("sensors"))
        parseInputs(jnet["sensors"], net.inputs, nullptr);

    net.resetReceptivies();
    return {};
}

//------------------------------------------------------------------------------
std::string importAllNetsFromJSON(std::vector<Net>& nets, std::string const& filename,
                                  DocumentInfo* info)
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

    // GRAFCET inputs shared between all nets of the document.
    if (info != nullptr)
        *info = DocumentInfo{};
    if (json.contains("inputs"))
    {
        std::map<std::string, int> inputs;
        parseInputs(json["inputs"], inputs, (info != nullptr) ? &info->inputs : nullptr);
        for (auto& net : nets)
            net.inputs.insert(inputs.begin(), inputs.end());
    }
    if (info != nullptr)
    {
        info->description = json.value("description", "");
        if (json.contains("outputs"))
            parseOutputs(json["outputs"], info->outputs);
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
