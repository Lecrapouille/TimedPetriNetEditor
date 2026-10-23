// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/Grafcet.hpp"
#include "PetriNet/PetriNet.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <cmath>
#include <cstring>
#include <map>
#include <sstream>
#include <vector>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Quoted and escaped JSON string.
static std::string quote(std::string const& text)
{
    return nlohmann::json(text).dump();
}

//------------------------------------------------------------------------------
//! \brief Readable one line JSON: { "id": 0, "x": 200 } (spaces, and whole
//! numbers without ".0").
static std::string format(nlohmann::ordered_json const& value)
{
    if (value.is_object())
    {
        std::string text = "{ ";
        bool first = true;
        for (auto const& it : value.items())
        {
            text += (first ? "" : ", ") + quote(it.key()) + ": " + format(it.value());
            first = false;
        }
        return text + " }";
    }
    if (value.is_array())
    {
        std::string text = "[";
        for (size_t i = 0u; i < value.size(); ++i)
            text += (i ? ", " : "") + format(value[i]);
        return text + "]";
    }
    if (value.is_number_float())
    {
        double const number = value.get<double>();
        if ((number == std::floor(number)) && (std::fabs(number) < 1e15))
            return std::to_string(static_cast<long long>(number));
    }
    return value.dump();
}

//------------------------------------------------------------------------------
//! \brief One line per element, with a comma between elements.
static void writeList(std::ofstream& file, std::vector<nlohmann::ordered_json> const& items,
                      char const* indent)
{
    for (size_t i = 0u; i < items.size(); ++i)
    {
        file << "\n" << indent << format(items[i]) << ((i + 1u < items.size()) ? "," : "");
    }
}

//------------------------------------------------------------------------------
//! \brief GRAFCET inputs: [{ "name": ..., "kind": ..., "description": ...,
//! "initial": ... }]. The described inputs first, in their order, then the
//! other inputs read by the nets. Initial values are the ones of the nets.
static std::vector<nlohmann::ordered_json> inputsToJSON(std::map<std::string, int> const& inputs,
                                                        DocumentInfo const* info)
{
    std::vector<nlohmann::ordered_json> items;
    std::map<std::string, int> others = inputs;
    if (info != nullptr)
    {
        for (GrafcetInput const& input : info->inputs)
        {
            auto const it = others.find(input.name);
            int const initial = (it != others.end()) ? it->second : input.initial;
            if (it != others.end())
                others.erase(it);
            nlohmann::ordered_json item = { { "name", input.name },
                                            { "kind", inputKindToStr(input.kind) } };
            if (!input.description.empty())
                item["description"] = input.description;
            item["initial"] = initial;
            if (!input.group.empty())
                item["group"] = input.group;
            if (!input.color.empty())
                item["color"] = input.color;
            if (input.kind == GrafcetInput::Kind::Value)
            {
                item["min"] = input.min;
                item["max"] = input.max;
                if (!input.unit.empty())
                    item["unit"] = input.unit;
                if (!input.driven_by.empty())
                {
                    item["driven_by"] = input.driven_by;
                    item["rise"] = input.rise;
                    item["fall"] = input.fall;
                }
            }
            if (input.kind == GrafcetInput::Kind::Clock)
                item["period"] = input.period;
            items.push_back(item);
        }
    }
    for (auto const& it : others)
        items.push_back({ { "name", it.first }, { "initial", it.second } });
    return items;
}

//------------------------------------------------------------------------------
//! \brief GRAFCET outputs: [{ "name": ..., "kind": ..., "description": ... }].
static std::vector<nlohmann::ordered_json> outputsToJSON(DocumentInfo const& info)
{
    std::vector<nlohmann::ordered_json> items;
    for (GrafcetOutput const& output : info.outputs)
    {
        nlohmann::ordered_json item = { { "name", output.name },
                                        { "kind", outputKindToStr(output.kind) } };
        if (!output.description.empty())
            item["description"] = output.description;
        items.push_back(item);
    }
    return items;
}

//------------------------------------------------------------------------------
// Helper function to write a single net to JSON
static void writeNetToJSON(std::ofstream& file, Net const& net)
{
    file << "    {\n";
    file << "       \"name\": " << quote(net.name) << ",\n";
    if (!net.description.empty())
        file << "       \"description\": " << quote(net.description) << ",\n";

    // Places
    std::vector<nlohmann::ordered_json> items;
    size_t const capacity = Net::defaultCapacity(net.type());
    for (auto const& p: net.places())
    {
        nlohmann::ordered_json item = { { "id", p.id }, { "caption", p.caption },
                                        { "tokens", p.tokens } };
        if (p.capacity != capacity)
            item["capacity"] = p.capacity;
        items.push_back(item);
    }
    file << "       \"places\": [";
    writeList(file, items, "            ");

    // Transitions
    items.clear();
    for (auto const& t: net.transitions())
    {
        nlohmann::ordered_json item = { { "id", t.id }, { "caption", t.caption } };
        if (t.delay > 0.0f)
            item["delay"] = t.delay;
        if (t.priority != 0)
            item["priority"] = t.priority;
        items.push_back(item);
    }
    file << "\n       ],\n       \"transitions\": [";
    writeList(file, items, "            ");

    // Arcs
    items.clear();
    for (auto const& a: net.arcs())
    {
        nlohmann::ordered_json item = { { "from", a.from.key() }, { "to", a.to.key() } };
        if (a.from.type == Node::Type::Transition)
            item["duration"] = a.durationOrZero();
        if (a.kind != ArcKind::Normal)
            item["kind"] = to_str(a.kind);
        if (a.weight != 1u)
            item["weight"] = a.weight;
        items.push_back(item);
    }
    file << "\n       ],\n       \"arcs\": [";
    writeList(file, items, "            ");

    // GRAFCET actions
    items.clear();
    for (auto const& p: net.places())
    {
        for (auto const& action : p.actions)
        {
            nlohmann::ordered_json item = { { "place_id", p.id } };
            if (!action.name.empty() || action.forcings.empty())
            {
                item["qualifier"] = qualifierToStr(action.qualifier);
                item["color"] = ledColorToStr(action.color);
                item["name"] = action.name;
            }
            if (!action.script.empty())
                item["script"] = action.script;
            if (action.duration > 0.0f)
                item["duration"] = action.duration;
            if (!action.condition.empty())
                item["condition"] = action.condition;
            if (!action.forcings.empty())
            {
                nlohmann::ordered_json forcings = nlohmann::ordered_json::array();
                for (auto const& forcing : action.forcings)
                    forcings.push_back(forcingToStr(forcing));
                item["forcing"] = forcings;
            }
            items.push_back(item);
        }
    }
    file << "\n       ],\n       \"actions\": [";
    writeList(file, items, "            ");
    file << "\n       ],\n";

    // Drawing, apart from the structure: [x, y] of each node by identifier.
    file << "       \"layout\": {\n          \"places\": [";
    items.clear();
    for (auto const& p: net.places())
        items.push_back({ net.position(p).x, net.position(p).y });
    writeList(file, items, "             ");
    file << "\n          ],\n          \"transitions\": [";
    items.clear();
    for (auto const& t: net.transitions())
        items.push_back({ net.position(t).x, net.position(t).y });
    writeList(file, items, "             ");
    file << "\n          ]\n       }\n";

    file << "    }";
}

//------------------------------------------------------------------------------
//! \brief Write a document made of the given nets.
static std::string writeDocument(std::vector<Net const*> const& nets, std::string const& filename,
                                 DocumentInfo const* info)
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
    file << "{\n";
    file << "  \"revision\": 2,\n";
    file << "  \"type\": " << quote(to_str(nets[0]->type())) << ",\n";
    if ((info != nullptr) && !info->description.empty())
        file << "  \"description\": " << quote(info->description) << ",\n";
    file << "  \"nets\": [\n";

    // Write all nets
    std::map<std::string, int> inputs;
    for (size_t i = 0; i < nets.size(); ++i)
    {
        writeNetToJSON(file, *nets[i]);
        file << ((i + 1u < nets.size()) ? ",\n" : "\n");
        inputs.insert(nets[i]->inputs.begin(), nets[i]->inputs.end());
    }
    file << "  ],\n";

    // GRAFCET inputs (shared between all nets) with their initial value
    file << "  \"inputs\": [";
    writeList(file, inputsToJSON(inputs, info), "    ");
    file << "\n  ]";
    if ((info != nullptr) && !info->outputs.empty())
    {
        file << ",\n  \"outputs\": [";
        writeList(file, outputsToJSON(*info), "    ");
        file << "\n  ]";
    }
    file << "\n";

    file << "}\n";
    return {};
}

//------------------------------------------------------------------------------
std::string exportAllNetsToJSON(std::vector<Net> const& nets, std::string const& filename,
                                DocumentInfo const* info)
{
    std::vector<Net const*> pointers;
    for (auto const& net : nets)
        pointers.push_back(&net);
    return writeDocument(pointers, filename, info);
}

//------------------------------------------------------------------------------
std::string exportToJSON(Net const& net, std::string const& filename)
{
    return writeDocument({ &net }, filename, nullptr);
}

} // namespace tpne
