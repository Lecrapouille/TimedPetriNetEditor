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
//! \brief GRAFCET inputs: [{ "name": ..., "initial": ... }].
static std::vector<nlohmann::ordered_json> inputsToJSON(std::map<std::string, int> const& inputs)
{
    std::vector<nlohmann::ordered_json> items;
    for (auto const& it : inputs)
        items.push_back({ { "name", it.first }, { "initial", it.second } });
    return items;
}

//------------------------------------------------------------------------------
// Helper function to write a single net to JSON
static void writeNetToJSON(std::ofstream& file, Net const& net)
{
    file << "    {\n";
    file << "       \"name\": " << quote(net.name) << ",\n";

    // Places
    std::vector<nlohmann::ordered_json> items;
    for (auto const& p: net.places())
    {
        items.push_back({ { "id", p.id }, { "caption", p.caption }, { "tokens", p.tokens },
                          { "x", p.x }, { "y", p.y } });
    }
    file << "       \"places\": [";
    writeList(file, items, "            ");

    // Transitions
    items.clear();
    for (auto const& t: net.transitions())
    {
        nlohmann::ordered_json item = { { "id", t.id }, { "caption", t.caption },
                                        { "x", t.x }, { "y", t.y } };
        if (t.delay > 0.0f)
            item["delay"] = t.delay;
        items.push_back(item);
    }
    file << "\n       ],\n       \"transitions\": [";
    writeList(file, items, "            ");

    // Arcs
    items.clear();
    for (auto const& a: net.arcs())
    {
        nlohmann::ordered_json item = { { "from", a.from.key }, { "to", a.to.key } };
        if (a.from.type == Node::Type::Transition)
            item["duration"] = std::isnan(a.duration) ? 0.0f : a.duration;
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
            nlohmann::ordered_json item = {
                { "place_id", p.id }, { "qualifier", qualifierToStr(action.qualifier) },
                { "color", ledColorToStr(action.color) }, { "name", action.name },
                { "script", action.script }, { "duration", action.duration } };
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
    file << "\n       ]\n";

    file << "    }";
}

//------------------------------------------------------------------------------
//! \brief Write a document made of the given nets.
static std::string writeDocument(std::vector<Net const*> const& nets, std::string const& filename)
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
    file << "  \"revision\": 5,\n";
    file << "  \"type\": " << quote(to_str(nets[0]->type())) << ",\n";
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
    writeList(file, inputsToJSON(inputs), "    ");
    file << "\n  ]\n";

    file << "}\n";
    return {};
}

//------------------------------------------------------------------------------
std::string exportAllNetsToJSON(std::vector<Net> const& nets, std::string const& filename)
{
    std::vector<Net const*> pointers;
    for (auto const& net : nets)
        pointers.push_back(&net);
    return writeDocument(pointers, filename);
}

//------------------------------------------------------------------------------
std::string exportToJSON(Net const& net, std::string const& filename)
{
    return writeDocument({ &net }, filename);
}

} // namespace tpne
