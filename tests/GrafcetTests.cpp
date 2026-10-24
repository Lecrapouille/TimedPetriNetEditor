// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/GrafcetEngine.hpp"
#include "PetriNet/Simulation.hpp"
#include "PetriNet/Flowshop.hpp"
#include "MaxPlus/Algorithms.hpp"
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
    Place& p0 = net.addPlace(1u);
    Place& p1 = net.addPlace(0u);
    Place& p2 = net.addPlace(0u);
    Transition& t0 = net.addTransition();
    Transition& t1 = net.addTransition();
    Transition& t2 = net.addTransition();
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
//! \brief GRAFCET of the given steps (1: initial) and transitions
//! { upstream steps, downstream steps, receptivity }.
struct Link { std::vector<size_t> from; std::vector<size_t> to; std::string receptivity; };
static Net makeGrafcet(std::vector<size_t> const& steps, std::vector<Link> const& links)
{
    Net net(TypeOfNet::GRAFCET);
    net.name = "G";
    for (size_t const initial : steps)
        net.addPlace(initial);
    for (auto const& link : links)
    {
        Transition& t = net.addTransition();
        t.caption = link.receptivity;
        for (size_t const s : link.from)
            net.addArc(net.places()[s], net.transitions()[t.id]);
        for (size_t const s : link.to)
            net.addArc(net.transitions()[t.id], net.places()[s]);
    }
    return net;
}

//------------------------------------------------------------------------------
//! \brief IEC 60848 rule 1: the initial steps are active at the start.
TEST(TestGrafcetRules, Rule1InitialSituation)
{
    Net net = makeGrafcet({ 1u, 0u, 1u }, { { { 0u }, { 1u }, "a" } });
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 0));
    EXPECT_FALSE(active(net, 1));
    EXPECT_TRUE(active(net, 2));
}

//------------------------------------------------------------------------------
//! \brief Rules 2 and 3: a transition fires when all its upstream steps are
//! active and its receptivity is true; it deactivates all of them and
//! activates all its downstream steps (AND divergence and convergence).
TEST(TestGrafcetRules, Rule3ParallelSequences)
{
    // X0 -a-> (X1, X2); (X1, X2) -b-> X3; X1 -c-> X4 (leaves the AND).
    Net net = makeGrafcet({ 1u, 0u, 0u, 0u, 0u },
                          { { { 0u }, { 1u, 2u }, "a" },
                            { { 1u, 2u }, { 3u }, "b" },
                            { { 1u }, { 4u }, "c" } });
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_FALSE(active(net, 0));
    EXPECT_TRUE(active(net, 1));
    EXPECT_TRUE(active(net, 2));

    // X1 leaves: the AND convergence is no longer validated.
    engine.setInput("a", 0);
    engine.setInput("c", 1);
    engine.step(0.1f);
    EXPECT_FALSE(active(net, 1));
    EXPECT_TRUE(active(net, 2));
    EXPECT_TRUE(active(net, 4));
    engine.setInput("b", 1);
    engine.step(0.1f);
    EXPECT_FALSE(active(net, 3)) << "X1 is not active";
    EXPECT_TRUE(active(net, 2));
}

//------------------------------------------------------------------------------
//! \brief Rule 4: all the transitions that can fire fire together, even the
//! branches of an alternative (OR divergence) whose receptivities are both
//! true.
TEST(TestGrafcetRules, Rule4SimultaneousFiring)
{
    // X0 -a-> X1, X0 -a-> X2 (both true), X3 -a-> X4 (other sequence).
    Net net = makeGrafcet({ 1u, 0u, 0u, 1u, 0u },
                          { { { 0u }, { 1u }, "a" },
                            { { 0u }, { 2u }, "a" },
                            { { 3u }, { 4u }, "a" } });
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_FALSE(active(net, 0));
    EXPECT_TRUE(active(net, 1));
    EXPECT_TRUE(active(net, 2));
    EXPECT_TRUE(active(net, 4));
    EXPECT_EQ(engine.firings().size(), 3u);
}

//------------------------------------------------------------------------------
//! \brief Rule 5: a step activated and deactivated at the same time stays
//! active.
TEST(TestGrafcetRules, Rule5ActivatedAndDeactivatedStaysActive)
{
    // X0 -a-> X0 (loop on itself) and X0 -b-> X1.
    Net net = makeGrafcet({ 1u, 0u }, { { { 0u }, { 0u }, "↑a" },
                                        { { 0u }, { 1u }, "b" } });
    GrafcetEngine engine;
    engine.graphs({ &net });
    ASSERT_TRUE(engine.start());
    engine.step(0.1f);
    engine.setInput("a", 1);
    engine.step(0.1f);
    ASSERT_EQ(engine.firings().size(), 1u); // the loop fired
    EXPECT_TRUE(active(net, 0));
    EXPECT_FALSE(active(net, 1));
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

    // A restart begins from the initial input values: a rises again.
    engine.stop();
    ASSERT_TRUE(engine.start());
    EXPECT_EQ(engine.inputValue("a"), 0);
    engine.setInput("a", 1);
    engine.step(0.1f);
    EXPECT_TRUE(active(net, 1));
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
                              "../data/examples/GrafcetActions.json", "../data/examples/GEMMA-Dryer.json" })
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

//------------------------------------------------------------------------------
TEST(TestFlowshop, BuildScheduleAndCycleTime)
{
    // Two machines, two pieces, one token each: M1 then M2 for both pieces.
    Flowshop shop;
    shop.npieces = 2; shop.nmachines = 2;
    shop.pieceNames = { "P1", "P2" };
    shop.machineNames = { "M1", "M2" };
    shop.np = { 1, 1 }; shop.nm = { 1, 1 };
    shop.PT = { { 2.0f, 3.0f }, { 1.0f, 4.0f } };
    ASSERT_STREQ(checkFlowshop(shop).c_str(), "");

    Net net;
    buildFlowshop(net, shop);
    EXPECT_EQ(net.type(), TypeOfNet::TimedEventGraph);

    std::vector<std::vector<double>> dates;
    ASSERT_STREQ(earliestDates(net, 3, dates).c_str(), "");
    auto date = [&](std::string const& caption, size_t k) {
        for (size_t t = 0u; t < net.transitions().size(); ++t)
            if (net.transitions()[t].caption == caption)
                return dates[t][k];
        return -1.0;
    };
    // First cycle: M1_P1 at 0, M1_P2 at 2, M2_P1 at 2, M2_P2 at max(5, 3) = 5.
    EXPECT_DOUBLE_EQ(date("M1_P1", 0), 0.0);
    EXPECT_DOUBLE_EQ(date("M1_P2", 0), 2.0);
    EXPECT_DOUBLE_EQ(date("M2_P1", 0), 2.0);
    EXPECT_DOUBLE_EQ(date("M2_P2", 0), 5.0);

    // Cycle time: machine M2 works 1 + 4 = 5, M1 2 + 3 = 5, piece P2 3 + 4 = 7.
    CriticalCycleResult const cycle = findCriticalCycle(net);
    ASSERT_TRUE(cycle.success) << cycle.message.str();
    double lambda = 0.0;
    for (double d : cycle.durations)
        lambda = std::max(lambda, d);
    EXPECT_DOUBLE_EQ(lambda, 7.0);
    // Steady state: one P2 every 7 time units.
    EXPECT_DOUBLE_EQ(date("M1_P2", 2) - date("M1_P2", 1), 7.0);
}

//------------------------------------------------------------------------------
TEST(TestGrafcetJSON, ForcingWithoutColon)
{
    Forcing const a = parseForcing("Production{init}");
    EXPECT_EQ(a.targetNet, "Production");
    EXPECT_EQ(a.type, Forcing::Type::Init);
    Forcing const b = parseForcing("G2:{2, 8}");
    EXPECT_EQ(b.targetNet, "G2");
    ASSERT_EQ(b.steps.size(), 2u);
    EXPECT_EQ(b.steps[1], 8u);
    EXPECT_EQ(forcingLabel(b), "G2{2,8}");
    EXPECT_EQ(forcingToStr(parseForcing("Safety{*}")), "Safety:{*}");
}

//------------------------------------------------------------------------------
TEST(TestGrafcetJSON, DescriptionsInputsAndOutputs)
{
    // Revision 1: descriptions of the document, of the nets, of the inputs
    // and outputs (control panel).
    std::vector<Net> nets;
    DocumentInfo info;
    ASSERT_STREQ(importAllNetsFromJSON(nets, "data/Gemma.json", &info).c_str(), "");
    ASSERT_EQ(nets.size(), 3u);
    EXPECT_NE(info.description.find("séchoir"), std::string::npos);
    EXPECT_FALSE(nets[0].description.empty());
    GrafcetInput const* auto_ = findInput(info, "Auto");
    ASSERT_NE(auto_, nullptr);
    EXPECT_EQ(auto_->kind, GrafcetInput::Kind::Selector);
    EXPECT_EQ(auto_->group, "Mode");
    GrafcetInput const* temperature = findInput(info, "Temperature");
    ASSERT_NE(temperature, nullptr);
    EXPECT_EQ(temperature->kind, GrafcetInput::Kind::Value);
    EXPECT_EQ(temperature->unit, "°C");
    EXPECT_EQ(temperature->driven_by, "Four");
    EXPECT_EQ(nets[0].inputs.at("Temperature"), 20);
    ASSERT_NE(findOutput(info, "Four"), nullptr);
    EXPECT_EQ(findOutput(info, "Four")->kind, GrafcetOutput::Kind::Heater);
    // A forcing order without a name (drawn "Production{init}").
    tpne::Action const& forcing = nets[2].places()[0].actions[0];
    EXPECT_TRUE(forcing.name.empty());
    ASSERT_EQ(forcing.forcings.size(), 2u);
    EXPECT_EQ(forcingLabel(forcing.forcings[1]), "Controle{init}");

    // Saved and loaded again: the same.
    std::string const file = "/tmp/tpne-gemma-test.json";
    ASSERT_STREQ(exportAllNetsToJSON(nets, file, &info).c_str(), "");
    std::vector<Net> again;
    DocumentInfo info2;
    ASSERT_STREQ(importAllNetsFromJSON(again, file, &info2).c_str(), "");
    EXPECT_EQ(info2.description, info.description);
    EXPECT_EQ(again[1].description, nets[1].description);
    ASSERT_EQ(info2.inputs.size(), info.inputs.size());
    EXPECT_EQ(info2.inputs[0].name, "Au");
    EXPECT_EQ(info2.inputs[0].kind, GrafcetInput::Kind::Emergency);
    EXPECT_EQ(findInput(info2, "Temperature")->max, 120);
    EXPECT_FLOAT_EQ(findInput(info2, "Temperature")->rise, 6.0f);
    ASSERT_EQ(info2.outputs.size(), 3u);
    EXPECT_EQ(again[2].places()[0].actions[0].forcings.size(), 2u);
    EXPECT_EQ(again[1].places()[5].actions[0].condition, "Dr");
    std::ifstream saved(file);
    std::string const text((std::istreambuf_iterator<char>(saved)), std::istreambuf_iterator<char>());
    EXPECT_NE(text.find("\"revision\": 2"), std::string::npos);
    EXPECT_NE(text.find("\"layout\""), std::string::npos);
    std::remove(file.c_str());
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, GemmaDryer)
{
    std::vector<Net> nets;
    ASSERT_STREQ(importAllNetsFromJSON(nets, "data/Gemma.json").c_str(), "");
    Net& production = nets[0];
    Net& control = nets[1];
    Net& safety = nets[2];
    GrafcetEngine engine;
    engine.graphs({ &production, &control, &safety });
    ASSERT_TRUE(engine.start());
    auto const active = [](Net& net, size_t id) { return net.findPlace(id)->tokens > 0u; };
    auto const step = [&engine](float dt = 0.1f) { engine.step(dt); };

    // Forced by the safety until rearmed.
    step();
    EXPECT_TRUE(active(safety, 0));
    engine.setInput("Rearm", 1); step(); engine.setInput("Rearm", 0);
    EXPECT_TRUE(active(safety, 1));

    // Validation, then automatic mode: the oven heats.
    engine.setInput("Val", 1); step(); engine.setInput("Val", 0); step();
    EXPECT_TRUE(active(control, 2));
    engine.setInput("Val", 1); step(); engine.setInput("Val", 0);
    EXPECT_TRUE(active(control, 3));
    EXPECT_TRUE(engine.output("Four"));
    engine.setInput("Temperature", 85); step();
    EXPECT_TRUE(active(control, 4));
    EXPECT_FALSE(engine.output("Four"));

    // Cycle: left motor 30 s, drying until Acy, right motor 30 s.
    engine.setInput("Dcy", 1); step(); engine.setInput("Dcy", 0);
    EXPECT_TRUE(active(production, 1));
    EXPECT_TRUE(engine.output("Moteur Gauche"));
    for (int i = 0; i < 301; ++i) step();
    EXPECT_TRUE(active(production, 2));
    EXPECT_FALSE(engine.output("Moteur Gauche"));
    engine.setInput("Acy", 1); step(); engine.setInput("Acy", 0);
    EXPECT_TRUE(active(production, 3));
    EXPECT_TRUE(engine.output("Moteur Droit"));
    for (int i = 0; i < 301; ++i) step();
    EXPECT_TRUE(active(production, 0));

    // Manual mode: Dr drives the right motor, the production is forced.
    engine.setInput("Auto", 0); engine.setInput("Manu", 1);
    engine.setInput("Val", 1); step(); engine.setInput("Val", 0);
    EXPECT_TRUE(active(control, 5));
    EXPECT_FALSE(engine.output("Moteur Droit"));
    engine.setInput("Dr", 1); step();
    EXPECT_TRUE(engine.output("Moteur Droit"));
    EXPECT_FALSE(engine.output("Moteur Gauche"));

    // Emergency stop: everything back to its initial situation.
    engine.setInput("Au", 1); step(); step();
    EXPECT_TRUE(active(safety, 0));
    EXPECT_TRUE(active(control, 0));
    EXPECT_FALSE(engine.output("Moteur Droit"));
}

//------------------------------------------------------------------------------
TEST(TestSimulation, FiringDatesAndTraces)
{
    // The firing dates (plot of the analysis) and the traces (console) of a
    // timed event graph and of a flowshop, written like the Gantt chart.
    for (char const* file : { "../data/examples/EventGraph.json", "../data/examples/simple.flowshop" })
    {
        Net net;
        std::string const error = (std::string(file).find(".flowshop") != std::string::npos)
            ? importFlowshop(net, file) : importFromJSON(net, file);
        ASSERT_STREQ(error.c_str(), "") << file;
        Simulation simulation(net);
        std::vector<std::string> traces;
        simulation.onTrace.connect([&traces](std::string const& text) { traces.push_back(text); });
        simulation.start();
        for (int i = 0; i < 400; ++i)
            simulation.step(0.05f);
        EXPECT_GT(simulation.totalFirings(), 0u) << file;
        ASSERT_FALSE(traces.empty()) << file;
        EXPECT_EQ(traces.front().rfind("t = ", 0), 0u) << traces.front();
        EXPECT_NE(traces.front().find("(k = 1"), std::string::npos) << traces.front();
        simulation.stop();
        simulation.step(0.05f);
    }
}

//------------------------------------------------------------------------------
TEST(TestGrafcetEngine, RemoveStepKeepsActionsAndReferences)
{
    std::vector<Net> nets;
    ASSERT_STREQ(importAllNetsFromJSON(nets, "data/Gemma.json").c_str(), "");
    Net& production = nets[0];

    // Step 2 removed: step 3 takes the number 2 with its action, and the
    // receptivity 30s/X3 follows it; what read X2 reads a missing step.
    production.transitions()[2].caption = "Acy . /X2";
    production.removeNode(*production.findPlace(2));
    ASSERT_EQ(production.places().size(), 3u);
    Place const& moved = *production.findPlace(2);
    EXPECT_EQ(moved.caption, "Arrêt");
    ASSERT_EQ(moved.actions.size(), 1u);
    EXPECT_EQ(moved.actions[0].name, "Moteur Droit");
    EXPECT_EQ(production.findPlace(1)->actions[0].name, "Moteur Gauche");
    std::vector<std::string> captions;
    for (auto const& t : production.transitions())
        captions.push_back(t.caption);
    EXPECT_NE(std::find(captions.begin(), captions.end(), "30s/X2"), captions.end());
    EXPECT_NE(std::find(captions.begin(), captions.end(), "Acy . /X4"), captions.end());
    EXPECT_NE(std::find(captions.begin(), captions.end(), "Controle:X4 . Dcy"), captions.end());

    // Other graphs: "Controle:X5" follows, local "X5" of another graph not.
    Net& control = nets[1];
    production.transitions()[0].caption = "Controle:X5 . X5";
    control.removeNode(*control.findPlace(1));
    production.renameStepOf("Controle", 1, 6);
    production.renameStepOf("Controle", 5, 1);
    EXPECT_EQ(production.transitions()[0].caption, "Controle:X1 . X5");
    EXPECT_EQ(renameStepReference("Safety:X3 + X3 + X31 + AX3", "", 3, 7), "Safety:X3 + X7 + X31 + AX3");
}
