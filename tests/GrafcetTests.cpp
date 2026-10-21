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

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/GrafcetEngine.hpp"
#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/Imports/Imports.hpp"

#include <cstdio>
#include <fstream>

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief GRAFCET X0 -(r0)-> X1 -(r1)-> X2 -(r2)-> X0, X0 initial.
static Net makeSequence(std::string const& r0, std::string const& r1,
                        std::string const& r2, std::string const& name = "G")
{
    Net net(TypeOfNet::GRAFCET);
    net.name = name;
    Place& p0 = net.addPlace(0.0f, 0.0f, 1u);
    Place& p1 = net.addPlace(0.0f, 100.0f, 0u);
    Place& p2 = net.addPlace(0.0f, 200.0f, 0u);
    Transition& t0 = net.addTransition(0.0f, 50.0f);
    Transition& t1 = net.addTransition(0.0f, 150.0f);
    Transition& t2 = net.addTransition(100.0f, 100.0f);
    t0.caption = r0; t1.caption = r1; t2.caption = r2;
    net.addArc(p0, t0); net.addArc(t0, p1);
    net.addArc(p1, t1); net.addArc(t1, p2);
    net.addArc(p2, t2); net.addArc(t2, p0);
    return net;
}

//------------------------------------------------------------------------------
static bool active(Net const& net, size_t const id)
{
    for (auto const& place : net.places())
    {
        if (place.id == id)
            return place.tokens > 0u;
    }
    return false;
}

//------------------------------------------------------------------------------
TEST(TestReceptivity, Syntax)
{
    EXPECT_STREQ(Receptivity::check("").c_str(), "");
    EXPECT_STREQ(Receptivity::check("a . b + /c").c_str(), "");
    EXPECT_STREQ(Receptivity::check("X1 . Safety:X3 . (a + !b)").c_str(), "");
    EXPECT_STREQ(Receptivity::check("↑a . ↓b + ^c + rising(d) . falling(e)").c_str(), "");
    EXPECT_STREQ(Receptivity::check("3s/X2 + 2s/a/500ms + 1min/b").c_str(), "");
    EXPECT_STREQ(Receptivity::check("[count >= 3] . [level != -2]").c_str(), "");
    EXPECT_STREQ(Receptivity::check("done(Grasp(red_cube)) . holding(red_cube)").c_str(), "");
    EXPECT_STREQ(Receptivity::check("1 . 0 + true . not false").c_str(), "");

    EXPECT_STREQ(Receptivity::check("a .").c_str(),
                 "Syntax error in 'a .' at column 4: unexpected end, an operand is expected");
    EXPECT_STREQ(Receptivity::check("(a + b").c_str(),
                 "Syntax error in '(a + b' at column 7: missing ')'");
    EXPECT_STREQ(Receptivity::check("3/a").c_str(),
                 "Syntax error in '3/a' at column 2: only 0 and 1 are boolean constants; a duration needs a unit (ms, s, min, h)");
    EXPECT_STREQ(Receptivity::check("a b").c_str(),
                 "Syntax error in 'a b' at column 3: unexpected 'b'");
}

//------------------------------------------------------------------------------
TEST(TestReceptivity, TranslateAndInputs)
{
    bool supported;
    EXPECT_STREQ(Receptivity::translate("a . /b + X3", "C", supported).c_str(),
                 "((a && !(b)) || X[3])");
    EXPECT_TRUE(supported);
    EXPECT_STREQ(Receptivity::translate("a . /b + X3", "ST", supported).c_str(),
                 "((a AND NOT (b)) OR X[3])");
    EXPECT_STREQ(Receptivity::translate("", "C", supported).c_str(), "true");
    Receptivity::translate("↑a", "C", supported);
    EXPECT_FALSE(supported);

    auto const inputs = Receptivity::inputs("a . X2 + [n > 2] + 3s/b + Safety:X1");
    EXPECT_EQ(inputs, (std::set<std::string>{ "a", "b", "n" }));
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, SequenceAndPrecedence)
{
    Net net = makeSequence("a . b + c", "/a", "");
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    EXPECT_EQ(engine.inputs().size(), 3u);

    engine.step(0.1f);
    EXPECT_TRUE(active(net, 0));

    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 0)) << "a . b + c is false";

    engine.setInput("b", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 1));
    EXPECT_FALSE(active(net, 0));

    // /a then the empty receptivity: two evolutions in one cycle (stability
    // search), back to X0 where a . b is still... false since a = 0.
    engine.setInput("a", 0);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 0));
    EXPECT_EQ(engine.firings().size(), 2u);

    engine.stop();
    EXPECT_TRUE(active(net, 0));
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, EdgesLastOneInstant)
{
    // X0 -↑a-> X1 -↑a-> X2: one rising edge crosses only one transition.
    Net net = makeSequence("↑a", "↑a", "0");
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());

    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 1));
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 1)) << "a stays true: no new edge";
    engine.setInput("a", 0);
    engine.step(0.1f);
    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 2));
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, TimeOperator)
{
    Net net = makeSequence("go", "2s/X1", "0");
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.setInput("go", 1);
    engine.step(0.5f);
    ASSERT_TRUE(active(net, 1));
    for (int i = 0; i < 3; ++i)
    {
        engine.step(0.5f);
        EXPECT_TRUE(active(net, 1)) << "t = " << engine.time();
    }
    engine.step(0.5f);
    EXPECT_TRUE(active(net, 2)) << "2 s after X1 became active";
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, Actions)
{
    Net net = makeSequence("go", "next", "back");
    Place& x0 = net.places()[0];
    Place& x1 = net.places()[1];
    Place& x2 = net.places()[2];
    tpne::Action action;
    action.name = "Motor"; action.qualifier = tpne::Action::Qualifier::N;
    x1.actions.push_back(action);
    action.name = "Lamp"; action.qualifier = tpne::Action::Qualifier::S;
    x1.actions.push_back(action);
    action.name = "Lamp"; action.qualifier = tpne::Action::Qualifier::R;
    x0.actions.push_back(action);
    action.name = "Horn"; action.qualifier = tpne::Action::Qualifier::D; action.duration = 1.0f;
    x2.actions.push_back(action);
    action.name = "Bip"; action.qualifier = tpne::Action::Qualifier::P0; action.duration = 0.0f;
    x2.actions.push_back(action);
    action.name = "Fan"; action.qualifier = tpne::Action::Qualifier::N; action.condition = "hot";
    x2.actions.push_back(action);

    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.step(0.5f);
    EXPECT_FALSE(engine.output("Motor"));

    engine.setInput("go", 1);
    engine.step(0.5f);
    EXPECT_TRUE(engine.output("Motor"));
    EXPECT_TRUE(engine.output("Lamp"));

    engine.setInput("next", 1);
    engine.step(0.5f);
    EXPECT_FALSE(engine.output("Motor"));
    EXPECT_TRUE(engine.output("Lamp")) << "stored";
    EXPECT_FALSE(engine.output("Horn"));
    EXPECT_FALSE(engine.output("Fan"));
    engine.setInput("hot", 1);
    engine.step(0.5f);
    EXPECT_FALSE(engine.output("Horn"));
    EXPECT_TRUE(engine.output("Fan"));
    engine.step(0.5f);
    EXPECT_TRUE(engine.output("Horn")) << "1 s after activation";

    engine.setInput("go", 0);
    engine.setInput("back", 1);
    engine.step(0.5f);
    EXPECT_TRUE(active(net, 0));
    EXPECT_TRUE(engine.output("Bip"));
    EXPECT_FALSE(engine.output("Lamp")) << "reset by X0";
    engine.step(0.5f);
    EXPECT_FALSE(engine.output("Bip")) << "pulse";
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, ForcingAndCrossGraph)
{
    // Safety: X0 (run) -au-> X1 (forces Production to its initial situation)
    Net safety = makeSequence("au", "/au", "0", "Safety");
    tpne::Action forcing;
    forcing.name = "F/Production";
    forcing.forcings.push_back(parseForcing("Production:{init}"));
    safety.places()[1].actions.push_back(forcing);

    Net production = makeSequence("dcy . Safety:X0", "1", "0", "Production");
    production.places()[2].tokens = 0u;

    GrafcetEngine engine;
    engine.graphs({ &safety, &production });
    ASSERT_TRUE(engine.start()) << engine.errors().front();

    engine.setInput("dcy", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(production, 2)) << "X0 -> X1 -> X2 in one cycle";

    engine.setInput("au", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(safety, 1));
    engine.step(0.1f);
    EXPECT_TRUE(engine.isForced(production));
    EXPECT_TRUE(active(production, 0)) << "forced to its initial situation";
    EXPECT_FALSE(active(production, 2));
    engine.step(0.1f);
    EXPECT_TRUE(active(production, 0)) << "forced graphs do not evolve";
    EXPECT_FALSE(engine.output("F/Production")) << "forcing orders are not outputs";

    engine.setInput("au", 0);
    engine.step(0.1f);
    engine.step(0.1f);
    EXPECT_FALSE(engine.isForced(production));
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, CompileErrorsAndIndependentEngines)
{
    Net bad = makeSequence("a .", "Unknown:X1", "X9");
    GrafcetEngine engine;
    engine.graphs({ &bad });
    ASSERT_FALSE(engine.start());
    ASSERT_EQ(engine.errors().size(), 3u);
    EXPECT_FALSE(engine.isRunning());

    // Two engines on two copies do not share inputs.
    Net n1 = makeSequence("a", "0", "0");
    Net n2 = makeSequence("a", "0", "0");
    GrafcetEngine e1, e2;
    e1.graphs({ &n1 }); e2.graphs({ &n2 });
    ASSERT_TRUE(e1.start()); ASSERT_TRUE(e2.start());
    e1.setInput("a", 1);
    e1.step(0.1f); e2.step(0.1f);
    EXPECT_TRUE(active(n1, 1));
    EXPECT_TRUE(active(n2, 0));
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, UnstableLoopIsReported)
{
    Net net = makeSequence("1", "1", "1");
    GrafcetEngine engine;
    std::string warning;
    engine.onWarning = [&warning](std::string const& w) { warning = w; };
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.step(0.1f);
    EXPECT_NE(warning.find("stable"), std::string::npos);
}

//------------------------------------------------------------------------------
TEST(TestGrafcetJSON, SaveLoadAndOldRevision)
{
    Net net = makeSequence("a . \"b\"", "3s/X1", "0");
    net.transitions()[0].caption = "a . /b";
    net.transitions()[1].delay = 1.5f;
    net.places()[1].actions.push_back(tpne::Action{});
    net.places()[1].actions[0].name = "Motor";
    net.places()[1].actions[0].condition = "hot";
    net.inputs["a"] = 1;

    std::string const file = "/tmp/tpne-grafcet-test.json";
    ASSERT_STREQ(exportToJSON(net, file).c_str(), "");
    std::vector<Net> nets;
    ASSERT_STREQ(importAllNetsFromJSON(nets, file).c_str(), "");
    ASSERT_EQ(nets.size(), 1u);
    EXPECT_EQ(nets[0].transitions()[0].caption, "a . /b");
    EXPECT_FLOAT_EQ(nets[0].transitions()[1].delay, 1.5f);
    EXPECT_EQ(nets[0].places()[1].actions[0].condition, "hot");
    EXPECT_EQ(nets[0].inputs.at("a"), 1);

    // Revision 4 files: reverse polish notation converted to infix.
    std::ofstream old(file);
    old << R"({ "revision": 4, "type": "GRAFCET", "nets": [ { "name": "G",
        "places": [ { "id": 0, "caption": "", "tokens": 1, "x": 0, "y": 0 } ],
        "transitions": [ { "id": 0, "caption": "A B ! .", "x": 0, "y": 0 },
                         { "id": 1, "caption": "a b + c .", "x": 0, "y": 0 },
                         { "id": 2, "caption": "Controle:X4 Dcy .", "x": 0, "y": 0 },
                         { "id": 3, "caption": "Start", "x": 0, "y": 0 } ],
        "arcs": [] } ] })";
    old.close();
    ASSERT_STREQ(importAllNetsFromJSON(nets, file).c_str(), "");
    EXPECT_EQ(nets[0].transitions()[0].caption, "A . /B");
    EXPECT_EQ(nets[0].transitions()[1].caption, "(a + b) . c");
    EXPECT_EQ(nets[0].transitions()[2].caption, "Controle:X4 . Dcy");
    EXPECT_EQ(nets[0].transitions()[3].caption, "Start");
    std::remove(file.c_str());
}

//------------------------------------------------------------------------------
TEST(TestGrafcetJSON, ExamplesCompileAndRun)
{
    for (char const* file : { "../data/examples/GRAFCET.json", "../data/examples/GRAFCET2.json",
                              "../data/examples/GermanTrafficLights.json",
                              "../data/examples/GrafcetActions.json", "../data/examples/Gemma.json" })
    {
        std::vector<Net> nets;
        ASSERT_STREQ(importAllNetsFromJSON(nets, file).c_str(), "") << file;
        std::vector<Net*> graphs;
        for (auto& net : nets)
            graphs.push_back(&net);
        GrafcetEngine engine;
        engine.graphs(graphs);
        ASSERT_TRUE(engine.start()) << file << ": " << engine.errors().front();
        for (int i = 0; i < 10; ++i)
            engine.step(0.1f);
    }
}
