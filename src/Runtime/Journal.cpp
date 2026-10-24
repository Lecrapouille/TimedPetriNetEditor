// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Runtime/Journal.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
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
std::string fromJsonLine(std::string const& line, ColorTable const& colors, LogEntry& entry)
{
    nlohmann::json const json = nlohmann::json::parse(line, nullptr, false);
    if (json.is_discarded() || !json.is_object() || !json.contains("transition") ||
        !json.contains("event") || !json["transition"].is_string() || !json["event"].is_string())
    {
        return "not an entry";
    }
    entry = LogEntry{};
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
        return "unknown event '" + event + "'";
    if (json.contains("result") && json["result"].is_string())
    {
        std::string const name = json["result"].get<std::string>();
        std::optional<Color> const color = colors.find(name);
        if (!color)
            return "unknown color '" + name + "'";
        entry.result = *color;
    }
    return {};
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
        LogEntry entry;
        std::string const error = fromJsonLine(line, colors, entry);
        if (!error.empty())
            return "Line " + std::to_string(number) + " of the journal: " + error;
        journal.push_back(entry);
    }
    return {};
}

//------------------------------------------------------------------------------
//! \brief The text as a CSV field: quoted when it holds a comma, a quote or
//! a new line.
static std::string csv(std::string const& text)
{
    if (text.find_first_of(",\"\n") == std::string::npos)
        return text;
    std::string s = "\"";
    for (char const c : text)
        s += (c == '"') ? std::string("\"\"") : std::string(1, c);
    return s + "\"";
}

//------------------------------------------------------------------------------
std::string toCsv(std::vector<LogEntry> const& journal, ColorTable const& colors)
{
    std::ostringstream text;
    text << "instance,transition,event,result,date\n";
    for (auto const& entry : journal)
    {
        text << csv(entry.instance) << "," << csv(entry.transition) << "," << to_str(entry.event) << ","
             << ((entry.event == LogEntry::Event::Completed) ? csv(colors.name(entry.result)) : "") << ","
             << entry.date << "\n";
    }
    return text.str();
}

//------------------------------------------------------------------------------
//! \brief The text escaped for XML.
static std::string xml(std::string const& text)
{
    std::string s;
    for (char const c : text)
    {
        switch (c)
        {
        case '&': s += "&amp;"; break;
        case '<': s += "&lt;"; break;
        case '>': s += "&gt;"; break;
        case '"': s += "&quot;"; break;
        default: s += c; break;
        }
    }
    return s;
}

//------------------------------------------------------------------------------
//! \brief ISO 8601 date (UTC, milliseconds) of seconds since 1970.
static std::string isoDate(double const seconds)
{
    double const whole = std::floor(seconds);
    std::time_t const t = static_cast<std::time_t>(whole);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char date[32];
    std::strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", &tm);
    char ms[8];
    std::snprintf(ms, sizeof(ms), ".%03dZ", int(std::lround((seconds - whole) * 1000.0)) % 1000);
    return std::string(date) + ms;
}

//------------------------------------------------------------------------------
std::string toXes(std::vector<LogEntry> const& journal, ColorTable const& colors, double const origin)
{
    // Instances in the order they appear.
    std::vector<std::string> instances;
    for (auto const& entry : journal)
    {
        if (std::find(instances.begin(), instances.end(), entry.instance) == instances.end())
            instances.push_back(entry.instance);
    }

    std::ostringstream text;
    text << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<log xes.version=\"1849-2016\" xes.features=\"\" xmlns=\"http://www.xes-standard.org/\">\n"
         << "  <extension name=\"Concept\" prefix=\"concept\" uri=\"http://www.xes-standard.org/concept.xesext\"/>\n"
         << "  <extension name=\"Lifecycle\" prefix=\"lifecycle\" uri=\"http://www.xes-standard.org/lifecycle.xesext\"/>\n"
         << "  <extension name=\"Time\" prefix=\"time\" uri=\"http://www.xes-standard.org/time.xesext\"/>\n"
         << "  <extension name=\"Organizational\" prefix=\"org\" uri=\"http://www.xes-standard.org/org.xesext\"/>\n"
         << "  <classifier name=\"Activity\" keys=\"concept:name lifecycle:transition\"/>\n";
    for (auto const& instance : instances)
    {
        text << "  <trace>\n    <string key=\"concept:name\" value=\"" << xml(instance) << "\"/>\n";
        for (auto const& entry : journal)
        {
            if (entry.instance != instance)
                continue;
            char const* lifecycle = (entry.event == LogEntry::Event::Scheduled) ? "schedule"
                                  : (entry.event == LogEntry::Event::Started) ? "start" : "complete";
            text << "    <event>\n"
                 << "      <string key=\"concept:name\" value=\"" << xml(entry.transition) << "\"/>\n"
                 << "      <string key=\"lifecycle:transition\" value=\"" << lifecycle << "\"/>\n"
                 << "      <date key=\"time:timestamp\" value=\"" << isoDate(origin + entry.date) << "\"/>\n";
            if (entry.event == LogEntry::Event::Completed)
                text << "      <string key=\"org:resource\" value=\"" << xml(colors.name(entry.result)) << "\"/>\n";
            text << "    </event>\n";
        }
        text << "  </trace>\n";
    }
    text << "</log>\n";
    return text.str();
}

//------------------------------------------------------------------------------
std::string saveJournal(std::vector<LogEntry> const& journal, ColorTable const& colors,
                        std::string const& path, double const origin)
{
    std::string extension = path.substr(std::min(path.size(), path.find_last_of('.')));
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    std::string text;
    if (extension == ".csv")
        text = toCsv(journal, colors);
    else if (extension == ".xes")
        text = toXes(journal, colors, origin);
    else if ((extension == ".jsonl") || (extension == ".json") || (extension == ".log"))
        text = toJsonLines(journal, colors);
    else
        return "Unknown format of journal '" + extension + "' (.jsonl, .csv, .xes)";

    std::ofstream file(path);
    if (!(file << text))
        return "Cannot write the journal '" + path + "'";
    return {};
}

} // namespace tpne
