// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"
#include "Runtime/Journal.hpp"

#include <fstream>
#include <sstream>

using namespace ::tpne;

//------------------------------------------------------------------------------
static std::string read(std::string const& path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

//------------------------------------------------------------------------------
//! \brief The journal of the runtime in CSV and XES (process mining).
TEST(TestExports, JournalFormats)
{
    ColorTable colors;
    std::vector<LogEntry> journal = {
        { "order,1", "grasp", LogEntry::Event::Scheduled, Success, 0.5 },
        { "order,1", "grasp", LogEntry::Event::Completed, Failed, 1.25 },
        { "order 2", "place <arm>", LogEntry::Event::Started, Success, 2.0 },
    };
    std::string const csv = toCsv(journal, colors);
    ASSERT_STREQ(csv.c_str(),
                 "instance,transition,event,result,date\n"
                 "\"order,1\",grasp,Scheduled,,0.5\n"
                 "\"order,1\",grasp,Completed,Failed,1.25\n"
                 "order 2,place <arm>,Started,,2\n");

    std::string const xes = toXes(journal, colors, 1700000000.0);
    ASSERT_THAT(xes, HasSubstr("<log xes.version=\"1849-2016\""));
    ASSERT_THAT(xes, HasSubstr("<string key=\"concept:name\" value=\"order,1\"/>"));
    ASSERT_THAT(xes, HasSubstr("<string key=\"lifecycle:transition\" value=\"complete\"/>"));
    ASSERT_THAT(xes, HasSubstr("<string key=\"org:resource\" value=\"Failed\"/>"));
    ASSERT_THAT(xes, HasSubstr("<date key=\"time:timestamp\" value=\"2023-11-14T22:13:21.250Z\"/>"));
    ASSERT_THAT(xes, HasSubstr("value=\"place &lt;arm&gt;\""));
    size_t traces = 0u;
    for (size_t pos = xes.find("<trace>"); pos != std::string::npos; pos = xes.find("<trace>", pos + 1u))
        ++traces;
    ASSERT_EQ(traces, 2u);

    ASSERT_STREQ(saveJournal(journal, colors, "/tmp/tpne-journal.CSV").c_str(), "");
    ASSERT_STREQ(read("/tmp/tpne-journal.CSV").c_str(), csv.c_str());
    ASSERT_STREQ(saveJournal(journal, colors, "/tmp/tpne-journal.jsonl").c_str(), "");
    std::vector<LogEntry> back;
    ASSERT_STREQ(fromJsonLines(read("/tmp/tpne-journal.jsonl"), colors, back).c_str(), "");
    ASSERT_EQ(back.size(), 3u);
    ASSERT_EQ(back[1].result, Failed);
    ASSERT_THAT(saveJournal(journal, colors, "/tmp/tpne-journal.txt"), HasSubstr("Unknown format"));
}

