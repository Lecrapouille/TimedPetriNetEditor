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
std::string toManifest(Runtime const& runtime)
{
    Net const& net = runtime.net();
    nlohmann::ordered_json transitions = nlohmann::ordered_json::array();
    for (auto const& t : net.transitions())
        transitions.push_back(t.caption.empty() ? t.key : t.caption);
    nlohmann::ordered_json places = nlohmann::ordered_json::array();
    for (auto const& p : net.places())
        places.push_back(p.caption.empty() ? p.key : p.caption);
    nlohmann::ordered_json json = {
        { "type", "manifest" },
        { "net", net.name },
        { "places", places },
        { "transitions", transitions },
        { "unregistered", runtime.unregisteredTransitions() },
    };
    return json.dump();
}

//------------------------------------------------------------------------------
std::string fromManifest(std::string const& text, Manifest& manifest)
{
    nlohmann::json const json = nlohmann::json::parse(text, nullptr, false);
    if (json.is_discarded() || !json.is_object() || (json.value("type", "") != "manifest"))
        return "not a manifest";
    manifest = Manifest{};
    manifest.net = json.value("net", "");
    for (char const* key : { "places", "transitions", "unregistered" })
    {
        if (!json.contains(key) || !json[key].is_array())
            continue;
        std::vector<std::string>& names = (key[0] == 'p') ? manifest.places
                                        : (key[0] == 't') ? manifest.transitions
                                                          : manifest.unregistered;
        for (auto const& name : json[key])
        {
            if (name.is_string())
                names.push_back(name.get<std::string>());
        }
    }
    return {};
}

//------------------------------------------------------------------------------
std::string toMarkingMessage(std::string const& instance, double const date,
                             ColoredMarking const& marking, ColorTable const& colors)
{
    nlohmann::ordered_json places = nlohmann::ordered_json::array();
    for (size_t p = 0u; p < marking.places(); ++p)
    {
        nlohmann::ordered_json place = nlohmann::ordered_json::object();
        for (auto const& [color, tokens] : marking.tokens(p))
            place[colors.name(color)] = tokens;
        places.push_back(place);
    }
    nlohmann::ordered_json json = {
        { "type", "marking" },
        { "instance", instance },
        { "date", date },
        { "tokens", marking.counts() },
        { "colors", places },
    };
    return json.dump();
}

//------------------------------------------------------------------------------
std::string fromMarkingMessage(std::string const& text, ColorTable const& colors,
                               std::string& instance, double& date, ColoredMarking& marking)
{
    nlohmann::json const json = nlohmann::json::parse(text, nullptr, false);
    if (json.is_discarded() || !json.is_object() || (json.value("type", "") != "marking"))
        return "not a marking message";
    if (!json.contains("tokens") || !json["tokens"].is_array())
        return "marking message without tokens";
    instance = json.value("instance", "");
    date = json.value("date", 0.0);

    nlohmann::json const& tokens = json["tokens"];
    marking = ColoredMarking(tokens.size());
    if (!json.contains("colors"))
    {
        for (size_t p = 0u; p < tokens.size(); ++p)
        {
            if (tokens[p].is_number_unsigned() && (tokens[p].get<size_t>() > 0u))
                marking.add(p, Success, tokens[p].get<size_t>());
        }
    }
    else
    {
        nlohmann::json const& places = json["colors"];
        if (!places.is_array() || (places.size() != tokens.size()))
            return "colors and tokens of the marking differ in size";
        for (size_t p = 0u; p < places.size(); ++p)
        {
            if (!places[p].is_object())
                return "colors of the place " + std::to_string(p) + " are not an object";
            for (auto const& it : places[p].items())
            {
                std::optional<Color> const color = colors.find(it.key());
                if (!color)
                    return "unknown color '" + it.key() + "'";
                if (!it.value().is_number_unsigned())
                    return "tokens of the color '" + it.key() + "' are not a number";
                marking.add(p, *color, it.value().get<size_t>());
            }
        }
    }
    marking.takeTouched();
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
