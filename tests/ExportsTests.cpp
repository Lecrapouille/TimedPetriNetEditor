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
//! \brief The file formats are chosen by the longest extension the file ends
//! with.
TEST(TestExports, FindByExtension)
{
    ASSERT_NE(findExporter("net.json"), nullptr);
    ASSERT_STREQ(findExporter("a/b/net.drawio.xml")->format.c_str(), "Draw.io");
    ASSERT_STREQ(findExporter("net.symmetri.pnml")->format.c_str(), "PNML for Symmetri");
    ASSERT_STREQ(findExporter("NET.PNML")->format.c_str(), "Petri Net Markup Language");
    ASSERT_STREQ(findExporter("net.json~")->format.c_str(), "JSON");
    ASSERT_EQ(findExporter("net.unknown"), nullptr);
    ASSERT_NE(findImporter("net.pnml"), nullptr);
    ASSERT_EQ(findImporter("net"), nullptr);
}

//------------------------------------------------------------------------------
//! \brief PNML keeps the weights (inscriptions), the kinds of arcs, the
//! capacities, the durations and the type of net (toolspecific).
TEST(TestExports, PNMLRoundTrip)
{
    Net net(TypeOfNet::TimedPetriNet);
    ASSERT_STREQ(loadFromFile(net, "data/Extensions.json").c_str(), "");
    net.arcs()[3].duration = 2.5f;
    net.transitions()[0].priority = 3;
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-extensions.pnml").c_str(), "");

    Net back;
    ASSERT_STREQ(loadFromFile(back, "/tmp/tpne-extensions.pnml").c_str(), "");
    ASSERT_EQ(back.type(), net.type());
    ASSERT_EQ(back.places().size(), 4u);
    ASSERT_EQ(back.transitions().size(), 1u);
    ASSERT_EQ(back.arcs().size(), 4u);
    ASSERT_EQ(back.tokens(), net.tokens());
    ASSERT_EQ(back.places()[2].capacity, 4u);
    ASSERT_EQ(back.transitions()[0].priority, 3);
    for (size_t i = 0u; i < net.arcs().size(); ++i)
    {
        Arc const& a = net.arcs()[i];
        Arc const* b = back.findArc(back.node(a.from), back.node(a.to));
        ASSERT_NE(b, nullptr);
        ASSERT_EQ(b->weight, a.weight);
        ASSERT_EQ(b->kind, a.kind);
        ASSERT_EQ(b->durationOrZero(), a.durationOrZero());
    }
    ASSERT_EQ(features(back), features(net));
}

//------------------------------------------------------------------------------
//! \brief Files of Symmetri: one initialMarking per token, colors on arcs.
TEST(TestExports, SymmetriPNML)
{
    Net net;
    ASSERT_STREQ(loadFromFile(net, "data/Symmetri.pnml").c_str(), "");
    ASSERT_EQ(net.places().size(), 3u);
    ASSERT_EQ(net.transitions().size(), 3u);
    ASSERT_EQ(net.places()[0].tokens, 2u);
    ASSERT_STREQ(net.transitions()[1].caption.c_str(), "place");
    Arc const* failed = net.findArc(net.places()[1], net.transitions()[2]);
    ASSERT_NE(failed, nullptr);
    ASSERT_EQ(failed->color, Failed);
    Arc const* done = net.findArc(net.transitions()[1], net.places()[2]);
    ASSERT_NE(done, nullptr);
    ASSERT_EQ(done->weight, 2u);

    // Written back for Symmetri: ids are the names, a token per marking.
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne.symmetri.pnml").c_str(), "");
    std::string const text = read("/tmp/tpne.symmetri.pnml");
    ASSERT_THAT(text, HasSubstr("<transition id=\"grasp\">"));
    ASSERT_THAT(text, HasSubstr("source=\"Held\" target=\"retry\" color=\"Failed\""));
    size_t markings = 0u;
    for (size_t pos = text.find("<initialMarking>"); pos != std::string::npos;
         pos = text.find("<initialMarking>", pos + 1u))
        ++markings;
    ASSERT_EQ(markings, 2u);
}

//------------------------------------------------------------------------------
//! \brief The files of the old versions of the editor wrote the duration in
//! the inscription of the arcs.
TEST(TestExports, LegacyPNML)
{
    {
        std::ofstream file("/tmp/tpne-legacy.pnml");
        file << "<?xml version=\"1.0\"?><pnml><net id=\"Timed Petri net\" type=\"x\"><page id=\"1\">"
             << "<place id=\"P0\"><name><text>A</text></name><initialMarking><text>1</text></initialMarking></place>"
             << "<transition id=\"T0\"><name><text>T</text></name></transition>"
             << "<arc id=\"P0T0\" source=\"P0\" target=\"T0\"><inscription><text>0</text></inscription></arc>"
             << "<arc id=\"T0P0\" source=\"T0\" target=\"P0\"><inscription><text>3.5</text></inscription></arc>"
             << "</page></net></pnml>";
    }
    Net net;
    ASSERT_STREQ(loadFromFile(net, "/tmp/tpne-legacy.pnml").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.arcs()[0].weight, 1u);
    ASSERT_EQ(net.arcs()[1].weight, 1u);
    ASSERT_EQ(net.arcs()[1].durationOrZero(), 3.5f);
}

//------------------------------------------------------------------------------
//! \brief A broken file gives an error instead of a crash.
TEST(TestExports, BadPNML)
{
    {
        std::ofstream file("/tmp/tpne-bad.pnml");
        file << "<pnml><net id=\"n\"><page><arc source=\"X\" target=\"Y\"/></page></net></pnml>";
    }
    Net net;
    ASSERT_THAT(loadFromFile(net, "/tmp/tpne-bad.pnml"), HasSubstr("unknown nodes"));
    {
        std::ofstream file("/tmp/tpne-bad.pnml");
        file << "<pnml><net";
    }
    ASSERT_THAT(loadFromFile(net, "/tmp/tpne-bad.pnml"), HasSubstr("Failed parsing"));
}

//------------------------------------------------------------------------------
//! \brief The program for the runtime: the net built in the code, a function
//! per transition, the results asked by the next arcs.
TEST(TestExports, RuntimeCpp)
{
    Net net;
    ASSERT_STREQ(loadFromFile(net, "../examples/Runtime/PickAndPlace/PickAndPlace.json").c_str(), "");
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-pick.cpp").c_str(), "");
    std::string const text = read("/tmp/tpne-pick.cpp");
    ASSERT_THAT(text, HasSubstr("namespace pick_and_place {"));
    ASSERT_THAT(text, HasSubstr("net.addPlace(1u, \"Ready\")"));
    ASSERT_THAT(text, HasSubstr("arc->color = net.colors.add(\"Failed\");"));
    ASSERT_THAT(text, HasSubstr("Color grasp()"));
    ASSERT_THAT(text, HasSubstr("Its result chooses what comes next: Failed (retry), Success (place)."));
    ASSERT_THAT(text, HasSubstr("runtime.on(\"grasp\", pick_and_place::grasp);"));
    ASSERT_THAT(text, HasSubstr("{ { \"Done\", Success } }"));
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

//------------------------------------------------------------------------------
//! \brief A net with captions that the formats shall escape, two transitions
//! of the same name, weights, an inhibitor arc, a duration and a color.
static Net trickyNet()
{
    Net net(TypeOfNet::TimedPetriNet);
    net.name = "Tricky \"net\"";
    Place& p0 = net.addPlace(2u, "a \"b\" <c> & d_e");
    Place& p1 = net.addPlace(0u, "Out");
    Transition& t0 = net.addTransition("go");
    Transition& t1 = net.addTransition("go");
    net.position(p0) = { 100.0f, 100.0f };
    net.position(p1) = { 300.0f, 100.0f };
    net.position(t0) = { 200.0f, 50.0f };
    net.position(t1) = { 200.0f, 150.0f };
    net.addArc(p0, t0);
    net.addArc(t0, p1, 1.5f);
    net.addArc(p1, t1);
    net.addArc(t1, p0);
    net.findArc(p0, t0)->weight = 2u;
    net.findArc(p1, t1)->kind = ArcKind::Inhibitor;
    net.findArc(t1, p0)->color = net.colors.add("Blue");
    return net;
}

//------------------------------------------------------------------------------
TEST(TestExports, EscapedAndComplete)
{
    Net const net = trickyNet();

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.dot").c_str(), "");
    std::string text = read("/tmp/tpne-tricky.dot");
    ASSERT_THAT(text, HasSubstr("digraph \"Tricky \\\"net\\\"\""));
    ASSERT_THAT(text, HasSubstr("label=\"a \\\"b\\\" <c> & d_e"));
    ASSERT_THAT(text, HasSubstr("P0 -> T0 [label=\"2\"]"));
    ASSERT_THAT(text, HasSubstr("T0 -> P1 [label=\"1.5 s\"]"));
    ASSERT_THAT(text, HasSubstr("P1 -> T1 [arrowhead=odot]"));
    ASSERT_THAT(text, HasSubstr("T1 -> P0 [label=\"Blue\"]"));

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.drawio.xml").c_str(), "");
    text = read("/tmp/tpne-tricky.drawio.xml");
    ASSERT_THAT(text, HasSubstr("value=\"a &quot;b&quot; &lt;c&gt; &amp; d_e\""));
    ASSERT_THAT(text, HasSubstr("endArrow=oval"));
    ASSERT_THAT(text, HasSubstr("x=\"80\" y=\"80\" width=\"40\"")); // centered on 100, 100

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.tex").c_str(), "");
    text = read("/tmp/tpne-tricky.tex");
    ASSERT_THAT(text, HasSubstr("label=below:{a \"b\" <c> \\& d\\_e}"));
    ASSERT_THAT(text, HasSubstr("(P0) at (2.00, -2.00)"));
    ASSERT_THAT(text, HasSubstr("\\draw[inhibitor] (P1) -- (T1);"));

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.yaml").c_str(), "");
    text = read("/tmp/tpne-tricky.yaml");
    ASSERT_THAT(text, HasSubstr("'go_t0':"));
    ASSERT_THAT(text, HasSubstr("'go_t1':"));
    ASSERT_THAT(text, HasSubstr("- 'a_b_c_d_e'"));
    ASSERT_THAT(text, HasSubstr("# Symfony does not know the weights, inhibitor arcs, colored arcs"));

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.svg").c_str(), "");
    text = read("/tmp/tpne-tricky.svg");
    ASSERT_THAT(text, HasSubstr("<title>Tricky &quot;net&quot;</title>"));
    ASSERT_THAT(text, HasSubstr("marker-end=\"url(#circle)\""));
}

//------------------------------------------------------------------------------
//! \brief GRML (GreatSPN, Symmetri): weights, inhibitor arcs, colors,
//! priorities and the durations of a timed net come back.
TEST(TestExports, GRMLRoundTrip)
{
    Net net = trickyNet();
    net.transitions()[0].priority = 2;
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-tricky.grml").c_str(), "");
    Net back;
    ASSERT_STREQ(loadFromFile(back, "/tmp/tpne-tricky.grml").c_str(), "");
    ASSERT_EQ(back.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(back.places()[0].tokens, 2u);
    ASSERT_STREQ(back.places()[0].caption.c_str(), "a \"b\" <c> & d_e");
    ASSERT_EQ(back.transitions()[0].priority, 2);
    ASSERT_EQ(back.findArc(back.places()[0], back.transitions()[0])->weight, 2u);
    ASSERT_EQ(back.findArc(back.places()[1], back.transitions()[1])->kind, ArcKind::Inhibitor);
    ASSERT_EQ(back.findArc(back.transitions()[0], back.places()[1])->durationOrZero(), 1.5f);
    ASSERT_STREQ(back.colors.name(back.findArc(back.transitions()[1], back.places()[0])->color).c_str(), "Blue");

    // Reset arcs are refused.
    Net extensions;
    ASSERT_STREQ(loadFromFile(extensions, "data/Extensions.json").c_str(), "");
    ASSERT_THAT(saveToFile(extensions, "/tmp/tpne-extensions.grml"), HasSubstr("no reset arcs"));
}
