// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "MaxPlus/Flowshop.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
static std::string trim(std::string const& str)
{
    size_t const first = str.find_first_not_of(" \t\r");
    if (first == std::string::npos)
        return {};
    return str.substr(first, str.find_last_not_of(" \t\r") - first + 1u);
}

//------------------------------------------------------------------------------
//! \brief Fields of a CSV line, as written: a comma between double quotes
//! does not split.
static std::vector<std::string> splitFields(std::string const& line)
{
    std::vector<std::string> fields(1u);
    bool quoted = false;
    for (char const c : line)
    {
        if (c == '"')
            quoted = !quoted;
        if ((c == ',') && !quoted)
            fields.emplace_back();
        else
            fields.back() += c;
    }
    return fields;
}

//------------------------------------------------------------------------------
//! \brief A name of the first row or column: name, name*n, "name"*n (between
//! double quotes it may hold a '*', a ',' or a '"' written "").
static std::string parseName(std::string const& field, std::string& name, size_t& count)
{
    std::string const text = trim(field);
    std::string rest;
    name.clear();
    count = 1u;
    if (!text.empty() && (text[0] == '"'))
    {
        size_t i = 1u;
        for (; i < text.size(); ++i)
        {
            if (text[i] != '"')
                name += text[i];
            else if ((i + 1u < text.size()) && (text[i + 1u] == '"'))
                name += text[++i];
            else
                break;
        }
        if (i >= text.size())
            return "Missing closing '\"' in " + text;
        rest = trim(text.substr(i + 1u));
    }
    else
    {
        size_t const star = text.rfind('*');
        name = trim(text.substr(0u, star));
        rest = (star == std::string::npos) ? std::string() : text.substr(star);
    }
    if (name.empty())
        return "A machine or a conveyor has no name";
    if (rest.empty())
        return {};
    std::string const number = trim(rest.substr(1u));
    char* end = nullptr;
    long const n = std::strtol(number.c_str(), &end, 10);
    if ((rest[0] != '*') || number.empty() || (*end != '\0') || (n < 0))
        return "'" + text + "': expected name*n with n the number of units";
    count = size_t(n);
    return {};
}

//------------------------------------------------------------------------------
//! \brief A cell: a processing time, or no operation (empty, -, -inf, eps,
//! %0, nan).
static std::string parseTime(std::string const& field, float& time)
{
    std::string const text = trim(field);
    if (text.empty() || (text == "-") || (text == "-inf") || (text == "eps") ||
        (text == "%0") || (text == "nan"))
    {
        time = flowshopNoOperation();
        return {};
    }
    char* end = nullptr;
    time = std::strtof(text.c_str(), &end);
    if ((end == text.c_str()) || (*end != '\0') || (time < 0.0f))
        return "'" + text + "' is not a processing time";
    return {};
}

//------------------------------------------------------------------------------
std::string parseFlowshopCSV(std::istream& input, Flowshop& shop)
{
    shop = Flowshop();
    std::string line;
    size_t row = 0u;
    bool header = true;
    while (std::getline(input, line))
    {
        ++row;
        std::string const content = trim(line);
        if (content.empty() || (content[0] == '#'))
            continue;

        std::vector<std::string> const fields = splitFields(content);
        std::string const where = "Line " + std::to_string(row) + ": ";
        if (header)
        {
            // Corner cell: a title, ignored.
            for (size_t c = 1u; c < fields.size(); ++c)
            {
                std::string name;
                size_t count;
                std::string const error = parseName(fields[c], name, count);
                if (!error.empty())
                    return where + error;
                shop.conveyorNames.push_back(name);
                shop.np.push_back(count);
            }
            header = false;
            continue;
        }

        if (fields.size() != shop.conveyorNames.size() + 1u)
            return where + std::to_string(fields.size() - 1u) + " processing times, expected " +
                   std::to_string(shop.conveyorNames.size()) + " (one per conveyor)";
        std::string name;
        size_t count;
        std::string error = parseName(fields[0], name, count);
        if (!error.empty())
            return where + error;
        shop.machineNames.push_back(name);
        shop.nm.push_back(count);
        std::vector<float> times;
        for (size_t c = 1u; c < fields.size(); ++c)
        {
            float time;
            error = parseTime(fields[c], time);
            if (!error.empty())
                return where + error;
            times.push_back(time);
        }
        shop.PT.push_back(times);
    }
    shop.nconveyors = shop.conveyorNames.size();
    shop.nmachines = shop.machineNames.size();
    return checkFlowshop(shop);
}

//------------------------------------------------------------------------------
//! \brief A name and its number of units, between double quotes when needed.
static std::string writeName(std::string const& name, size_t count)
{
    std::string text = name;
    if ((name.find_first_of(",\"*#") != std::string::npos) || (name != trim(name)))
    {
        text = "\"";
        for (char const c : name)
            text += (c == '"') ? std::string("\"\"") : std::string(1u, c);
        text += "\"";
    }
    if (count != 1u)
        text += "*" + std::to_string(count);
    return text;
}

//------------------------------------------------------------------------------
void writeFlowshopCSV(std::ostream& output, Flowshop const& shop)
{
    output << "# Flowshop: every part visits the machines from top to bottom, every\n"
              "# machine works on the conveyors from left to right.\n"
              "# First row: the conveyors, first column: the machines. Name*n: the\n"
              "# factory owns n of them (1 without *n). A name holding a ',', a '*'\n"
              "# or a '#' is written between double quotes: \"C*1\"*2.\n"
              "# Cells: processing time of the conveyor on the machine, in the time\n"
              "# unit of the net. Empty: the conveyor does not visit the machine.\n"
              "# Lines starting with '#' are comments.\n";
    output << "machine\\conveyor";
    for (size_t c = 0u; c < shop.nconveyors; ++c)
        output << ',' << writeName(shop.conveyorNames[c], shop.np[c]);
    output << '\n';
    for (size_t m = 0u; m < shop.nmachines; ++m)
    {
        output << writeName(shop.machineNames[m], shop.nm[m]);
        for (size_t c = 0u; c < shop.nconveyors; ++c)
        {
            output << ',';
            if (!flowshopIsNoOperation(shop.PT[m][c]))
                output << shop.PT[m][c];
        }
        output << '\n';
    }
}

//------------------------------------------------------------------------------
//! \brief Name of a transition: its caption, else its key.
static std::string const& nameOf(Transition const& t)
{
    return t.caption.empty() ? t.key : t.caption;
}

//------------------------------------------------------------------------------
//! \brief Order of the machines (or conveyors) given by the arcs between
//! operations, "a before b": the first known first among the ones free.
static bool orderOf(std::vector<std::string> const& known, std::set<std::pair<std::string, std::string>> const& before,
                    std::vector<std::string>& order)
{
    std::map<std::string, size_t> pending;
    for (auto const& name : known)
        pending[name] = 0u;
    for (auto const& it : before)
        ++pending[it.second];
    order.clear();
    while (order.size() < known.size())
    {
        auto const next = std::find_if(known.begin(), known.end(), [&](std::string const& name) {
            return (pending[name] == 0u) && (std::find(order.begin(), order.end(), name) == order.end());
        });
        if (next == known.end())
            return false; // A cycle: not a flowshop.
        order.push_back(*next);
        for (auto const& it : before)
        {
            if (it.first == *next)
                --pending[it.second];
        }
    }
    return true;
}

//------------------------------------------------------------------------------
std::string flowshopOfNet(Net const& net, Flowshop& shop)
{
    // The operations "<machine>_<conveyor>" made by buildFlowshop().
    struct Operation { std::string machine; std::string conveyor; };
    std::map<size_t, Operation> operations; // by transition id
    std::vector<std::string> machines;
    std::vector<std::string> conveyors;
    for (auto const& t : net.transitions())
    {
        Operation op;
        operationOf(nameOf(t), op.machine, op.conveyor);
        if (op.machine == op.conveyor)
            continue;
        if (std::find(machines.begin(), machines.end(), op.machine) == machines.end())
            machines.push_back(op.machine);
        if (std::find(conveyors.begin(), conveyors.end(), op.conveyor) == conveyors.end())
            conveyors.push_back(op.conveyor);
        operations[t.id] = op;
    }
    if (operations.empty())
        return "No operation <machine>_<conveyor>: not a flowshop";

    // Transitions after (or before) a transition, through its places.
    auto const neighbors = [&](Transition const& t, bool after) {
        std::vector<std::pair<Transition const*, Place const*>> result;
        for (auto const& arc : (after ? net.arcsOut(t) : net.arcsIn(t)))
        {
            Place const& place = net.placeOf(arc);
            for (auto const& next : (after ? net.arcsOut(place) : net.arcsIn(place)))
                result.push_back({ &net.transitionOf(next), &place });
        }
        return result;
    };

    std::set<std::pair<std::string, std::string>> machine_before;
    std::set<std::pair<std::string, std::string>> conveyor_before;
    std::map<std::pair<std::string, std::string>, float> times;
    std::map<std::string, size_t> np;
    std::map<std::string, size_t> nm;
    for (auto const& it : operations)
    {
        Transition const& t = net.transitions()[it.first];
        Operation const& op = it.second;
        auto const out = net.arcsOut(t);
        times[{ op.machine, op.conveyor }] = out.empty() ? 0.0f : out.front().durationOrZero();
        for (auto const& [next, place] : neighbors(t, true))
        {
            auto const found = operations.find(next->id);
            if (found == operations.end())
                continue;
            if (found->second.conveyor == op.conveyor)
                machine_before.insert({ op.machine, found->second.machine });
            if (found->second.machine == op.machine)
                conveyor_before.insert({ op.conveyor, found->second.conveyor });
        }
        // The resources wait in the places from their buffer transition.
        for (auto const& [previous, place] : neighbors(t, false))
        {
            if (operations.count(previous->id) != 0u)
                continue;
            if (nameOf(*previous) == op.conveyor)
                np[op.conveyor] = place->tokens;
            if (nameOf(*previous) == op.machine)
                nm[op.machine] = place->tokens;
        }
    }

    shop = Flowshop();
    if (!orderOf(machines, machine_before, shop.machineNames) ||
        !orderOf(conveyors, conveyor_before, shop.conveyorNames))
        return "The operations are not in the same order for all the parts: not a flowshop";
    shop.nmachines = shop.machineNames.size();
    shop.nconveyors = shop.conveyorNames.size();
    for (auto const& m : shop.machineNames)
    {
        if (nm.count(m) == 0u)
            return "No transition " + m + " giving the machine to its first operation: not a flowshop";
        shop.nm.push_back(nm[m]);
    }
    for (auto const& c : shop.conveyorNames)
    {
        if (np.count(c) == 0u)
            return "No transition " + c + " giving the conveyor to its first operation: not a flowshop";
        shop.np.push_back(np[c]);
    }
    for (auto const& m : shop.machineNames)
    {
        std::vector<float> row;
        for (auto const& c : shop.conveyorNames)
        {
            auto const found = times.find({ m, c });
            row.push_back((found == times.end()) ? flowshopNoOperation() : found->second);
        }
        shop.PT.push_back(row);
    }
    return checkFlowshop(shop);
}

//------------------------------------------------------------------------------
bool isFlowshop(Net const& net)
{
    Flowshop shop;
    return flowshopOfNet(net, shop).empty();
}

//------------------------------------------------------------------------------
std::string exportFlowshop(Net const& net, std::string const& filename)
{
    Flowshop shop;
    std::string const error = flowshopOfNet(net, shop);
    if (!error.empty())
        return error;
    std::ofstream file(filename);
    if (!file)
        return "Failed opening '" + filename + "'. Reason: '" + strerror(errno) + "'\n";
    writeFlowshopCSV(file, shop);
    return {};
}

} // namespace tpne
