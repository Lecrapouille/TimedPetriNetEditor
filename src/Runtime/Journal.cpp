// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Runtime/Journal.hpp"
#include "nlohmann/json.hpp"

#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
std::string to_str(LogEntry::Event const event)
{
    switch (event)
    {
    case LogEntry::Event::Scheduled: return "Scheduled";
    case LogEntry::Event::Started: return "Started";
    case LogEntry::Event::Completed: return "Completed";
    default: return "Unknown";
    }
}

//------------------------------------------------------------------------------
std::string toJsonLine(LogEntry const& entry, ColorTable const& colors)
{
    nlohmann::ordered_json line = {
        { "instance", entry.instance },
        { "transition", entry.transition },
        { "event", to_str(entry.event) },
    };
    if (entry.event == LogEntry::Event::Completed)
        line["result"] = colors.name(entry.result);
    line["date"] = entry.date;
    return line.dump();
}

//------------------------------------------------------------------------------
std::string toJsonLines(std::vector<LogEntry> const& journal, ColorTable const& colors)
{
    std::string text;
    for (auto const& entry : journal)
        text += toJsonLine(entry, colors) + "\n";
    return text;
}

//------------------------------------------------------------------------------
std::string fromJsonLines(std::string const& text, ColorTable const& colors,
                          std::vector<LogEntry>& journal)
{
    journal.clear();
    std::istringstream lines(text);
    std::string line;
    size_t number = 0u;
    while (std::getline(lines, line))
    {
        ++number;
        if (line.find_first_not_of(" \t\r") == std::string::npos)
            continue;
        nlohmann::json const json = nlohmann::json::parse(line, nullptr, false);
        if (json.is_discarded() || !json.is_object() || !json.contains("transition") ||
            !json.contains("event"))
        {
            return "Line " + std::to_string(number) + " of the journal is not an entry";
        }
        LogEntry entry;
        entry.instance = json.value("instance", "");
        entry.transition = json["transition"].get<std::string>();
        entry.date = json.value("date", 0.0);
        std::string const event = json["event"].get<std::string>();
        if (event == "Scheduled")
            entry.event = LogEntry::Event::Scheduled;
        else if (event == "Started")
            entry.event = LogEntry::Event::Started;
        else if (event == "Completed")
            entry.event = LogEntry::Event::Completed;
        else
            return "Line " + std::to_string(number) + " of the journal has an unknown event '" + event + "'";
        if (json.contains("result"))
        {
            std::string const name = json["result"].get<std::string>();
            std::optional<Color> const color = colors.find(name);
            if (!color)
                return "Line " + std::to_string(number) + " of the journal has an unknown color '" + name + "'";
            entry.result = *color;
        }
        journal.push_back(entry);
    }
    return {};
}

} // namespace tpne
