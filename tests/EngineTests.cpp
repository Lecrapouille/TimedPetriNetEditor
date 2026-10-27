// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Engine.hpp"
#include "MaxPlus/Flowshop.hpp"
#include "IO/Imports/Imports.hpp"

#include <set>

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief Timed event graph T0 -(3)-> P0 -> T1 -(2)-> P1 (1 token) -> T0.
static Net loop()
{
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "", 0u);
    Place& p1 = net.addPlace(1u, "", 1u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    net.addArc(t0, p0, 3.0f);
    net.addArc(p0, t1);
    net.addArc(t1, p1, 2.0f);
    net.addArc(p1, t0);
    return net;
}

//------------------------------------------------------------------------------
TEST(TestEngine, ExactDatesWhateverTheSteps)
{
    Net const net = loop();
    for (double const dt : { 0.01, 0.7, 4.0, 100.0 })
    {
        Engine engine(net);
        engine.reset();
        for (double t = 0.0; t < 20.0; t += dt)
            engine.advance(dt);
        auto const& dates = engine.firingDates();
        ASSERT_GE(dates[0].size(), 4u) << dt;
        ASSERT_EQ(dates[0][0], 0.0) << dt;
        ASSERT_EQ(dates[1][0], 3.0) << dt;
        ASSERT_EQ(dates[0][1], 5.0) << dt;
        ASSERT_EQ(dates[1][1], 8.0) << dt;
        ASSERT_EQ(dates[0][2], 10.0) << dt;
    }
    ASSERT_EQ(net.tokens(), Marking({ 0u, 1u })); // the net is not modified
}

//------------------------------------------------------------------------------
TEST(TestEngine, FlightsAndMarking)
{
    Net const net = loop();
    Engine engine(net);
    engine.reset();
    engine.advance(1.0);
    ASSERT_EQ(engine.marking(), Marking({ 0u, 0u }));
    ASSERT_EQ(engine.flights().size(), 1u);
    ASSERT_EQ(engine.flights()[0].arc, 0u);
    ASSERT_EQ(engine.flights()[0].start, 0.0);
    ASSERT_EQ(engine.flights()[0].end, 3.0);
    ASSERT_EQ(engine.nextEventDate(), 3.0);
    engine.advanceTo(3.0); // arrives and T1 fires at once
    ASSERT_EQ(engine.marking(), Marking({ 0u, 0u }));
    ASSERT_EQ(engine.flights()[0].arc, 2u);
    ASSERT_FALSE(engine.isDeadlocked());
}

//------------------------------------------------------------------------------
//! \brief The engine gives the earliest dates of the (max,+) dater equation.
TEST(TestEngine, SameDatesAsMaxPlus)
{
    Net net(TypeOfNet::TimedPetriNet);
    ASSERT_STREQ(importFlowshop(net, "../examples/Timed-Event-Graphs/Flowshop/simple.flowshop").c_str(), "");
    std::vector<std::vector<double>> expected;
    ASSERT_STREQ(earliestDates(net, 3u, expected).c_str(), "");

    Engine::Options options;
    options.policy = Engine::Policy::FirstDeclared;
    Engine engine(net, options);
    engine.reset();
    engine.advanceTo(1000.0);
    for (size_t t = 0u; t < expected.size(); ++t)
    {
        for (size_t k = 0u; k < expected[t].size(); ++k)
        {
            ASSERT_LT(k, engine.firingDates()[t].size()) << net.transitions()[t].caption;
            ASSERT_DOUBLE_EQ(engine.firingDates()[t][k], expected[t][k])
                << net.transitions()[t].caption << " k = " << k;
        }
    }
}

//------------------------------------------------------------------------------
//! \brief P0 (1 token) -> T0 -> P1 and P0 -> T1 -> P2: T0 and T1 conflict.
static Net conflict()
{
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "", 1u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Place& p2 = net.addPlace(2u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1, 1.0f);
    net.addArc(p0, t1);
    net.addArc(t1, p2, 1.0f);
    return net;
}

//------------------------------------------------------------------------------
static size_t winner(Net const& net, Engine::Options const& options)
{
    Engine engine(net, options);
    engine.reset();
    engine.advance(2.0);
    return (engine.marking()[1] == 1u) ? 0u : 1u;
}

//------------------------------------------------------------------------------
TEST(TestEngine, ConflictPolicies)
{
    Net net = conflict();
    Engine::Options options;

    options.policy = Engine::Policy::FirstDeclared;
    ASSERT_EQ(winner(net, options), 0u);

    options.policy = Engine::Policy::Priority;
    ASSERT_EQ(winner(net, options), 0u); // same priority: the first declared
    net.transitions()[1].priority = 5;
    ASSERT_EQ(winner(net, options), 1u);

    // Random: a seed gives always the same result; both happen.
    options.policy = Engine::Policy::Random;
    std::set<size_t> winners;
    for (uint64_t seed = 1u; seed <= 40u; ++seed)
    {
        options.seed = seed;
        size_t const w = winner(net, options);
        ASSERT_EQ(winner(net, options), w);
        winners.insert(w);
    }
    ASSERT_EQ(winners.size(), 2u);

    // Seed 0: drawn by the engine, given to replay.
    options.seed = 0u;
    Engine engine(net, options);
    engine.reset();
    ASSERT_NE(engine.seed(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestEngine, ManualMode)
{
    Net const net = conflict();
    Engine::Options options;
    options.manual = true;
    options.timed = false;
    Engine engine(net, options);
    engine.reset();
    engine.advance(10.0);
    ASSERT_EQ(engine.marking(), Marking({ 1u, 0u, 0u })); // nothing armed

    engine.arm(1u);
    engine.settle();
    ASSERT_EQ(engine.marking(), Marking({ 0u, 0u, 1u })); // untimed: at once
    ASSERT_FALSE(engine.isArmed(1u));
}

//------------------------------------------------------------------------------
TEST(TestEngine, EnablingDelay)
{
    Net net = conflict();
    net.transitions()[0].delay = 2.0f;
    net.transitions()[1].priority = -1;
    Engine::Options options;
    options.policy = Engine::Policy::Priority;
    net.transitions()[0].priority = 1;
    Engine engine(net, options);
    engine.reset();

    // T1 can fire at once, T0 only after 2: T1 takes the token.
    engine.advance(0.5);
    ASSERT_EQ(engine.firingDates()[1].size(), 1u);
    ASSERT_EQ(engine.firingDates()[0].size(), 0u);

    // Alone, T0 waits 2 before firing.
    net.transitions()[1].delay = 100.0f;
    Engine alone(net, options);
    alone.reset();
    alone.advance(1.0);
    ASSERT_EQ(alone.firingDates()[0].size(), 0u);
    ASSERT_DOUBLE_EQ(alone.enabledFor(0u), 1.0);
    alone.advance(5.0);
    ASSERT_EQ(alone.firingDates()[0], std::vector<double>({ 2.0 }));
}

//------------------------------------------------------------------------------
TEST(TestEngine, SourceTransitionsAndCapacity)
{
    // Source T0 -(4)-> P0 (capacity 2): fires at 0, 4, then P0 is full.
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "", 0u);
    p0.capacity = 2u;
    Transition& t0 = net.addTransition(0u, "");
    net.addArc(t0, p0, 4.0f);
    Engine engine(net);
    engine.reset();
    engine.advanceTo(100.0);
    ASSERT_EQ(engine.firingDates()[0], std::vector<double>({ 0.0, 4.0 }));
    ASSERT_EQ(engine.marking(), Marking({ 2u }));
    ASSERT_TRUE(engine.isDeadlocked());
}

//------------------------------------------------------------------------------
TEST(TestEngine, ZenoCycle)
{
    // T0 -> P0 -> T1 -> P1 (1 token) -> T0, all durations 0.
    Net net = loop();
    for (auto& arc : net.arcs())
        arc.duration = 0.0f;
    Engine::Options options;
    options.max_firings_per_date = 100u;
    Engine engine(net, options);
    std::string warning;
    engine.onWarning = [&warning](std::string const& w) { warning = w; };
    engine.reset();
    engine.advance(1.0);
    ASSERT_TRUE(engine.isZeno());
    ASSERT_TRUE(engine.isDeadlocked());
    ASSERT_NE(warning.find("without duration"), std::string::npos);
}

//------------------------------------------------------------------------------
TEST(TestEngine, Callbacks)
{
    Net const net = loop();
    Engine engine(net);
    std::vector<std::pair<size_t, double>> fired;
    size_t arrivals = 0u;
    engine.onFire = [&fired](size_t t, double date, size_t) { fired.push_back({ t, date }); };
    engine.onArrival = [&arrivals](Engine::Flight const&) noexcept { ++arrivals; };
    engine.reset();
    engine.advanceTo(5.0);
    ASSERT_EQ(fired.size(), 3u); // T0 at 0, T1 at 3, T0 at 5
    ASSERT_EQ(fired[2].first, 0u);
    ASSERT_EQ(fired[2].second, 5.0);
    ASSERT_EQ(arrivals, 2u);
}

//------------------------------------------------------------------------------
TEST(TestEngine, InjectedClock)
{
    Net const net = loop();
    Engine engine(net);
    engine.reset();
    double now = 0.0;
    TimeSource const clock = [&now]() noexcept { return now; };
    now = 6.0;
    engine.update(clock);
    ASSERT_EQ(engine.time(), 6.0);
    ASSERT_EQ(engine.firingDates()[0].size(), 2u);
}

//------------------------------------------------------------------------------
//! \brief P0 (1 token) -> T0 -> P1, P1 -(Success)-> T1 -> P2,
//! P1 -(Failed)-> T2 -> P3: the result of T0 chooses the way, as in the
//! runtime.
static Net coloredChoice()
{
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "", 1u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Place& p2 = net.addPlace(2u, "", 0u);
    Place& p3 = net.addPlace(3u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    Transition& t2 = net.addTransition(2u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1);
    net.addArc(p1, t1);
    net.addArc(t1, p2);
    net.addArc(p1, t2);
    net.addArc(t2, p3);
    net.findArc(p1, t1)->color = Success;
    net.findArc(p1, t2)->color = Failed;
    return net;
}

//------------------------------------------------------------------------------
TEST(TestEngine, ColoredResultChoosesTheWay)
{
    Net const net = coloredChoice();
    Engine::Options options;
    options.policy = Engine::Policy::FirstDeclared;
    options.results = Engine::Results::AlwaysSuccess;
    Engine engine(net, options);
    engine.reset();
    engine.advanceTo(1.0);
    ASSERT_EQ(engine.marking(), Marking({ 0u, 0u, 1u, 0u }));
    ASSERT_EQ(engine.coloredMarking().count(2u, Success), 1u);

    engine.onResult = [](size_t t) noexcept { return (t == 0u) ? Failed : Success; };
    engine.reset();
    engine.advanceTo(1.0);
    ASSERT_EQ(engine.marking(), Marking({ 0u, 0u, 0u, 1u }));
}

//------------------------------------------------------------------------------
TEST(TestEngine, ColoredResultsAskedByTheArcs)
{
    Net const net = coloredChoice();
    bool success = false, failed = false;
    for (uint64_t seed = 1u; seed <= 50u; ++seed)
    {
        Engine::Options options;
        options.seed = seed;
        Engine engine(net, options);
        engine.reset();
        engine.advanceTo(1.0);
        success |= (engine.marking()[2] == 1u);
        failed |= (engine.marking()[3] == 1u);
        ASSERT_EQ(engine.marking()[2] + engine.marking()[3], 1u);
    }
    ASSERT_TRUE(success && failed);
}

//------------------------------------------------------------------------------
//! \brief The result chosen for a transition in the editor (Transition::result).
TEST(TestEngine, ColoredResultOfTheTransition)
{
    Net net = coloredChoice();
    net.transitions()[0].result = Failed;
    for (uint64_t seed = 1u; seed <= 10u; ++seed)
    {
        Engine::Options options;
        options.seed = seed;
        Engine engine(net, options);
        engine.reset();
        engine.advanceTo(1.0);
        ASSERT_EQ(engine.marking(), Marking({ 0u, 0u, 0u, 1u }));
    }
}
