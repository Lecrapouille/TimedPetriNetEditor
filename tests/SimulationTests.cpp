// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"
#include "PetriNet/Simulation.hpp"

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief Start the simulation and run it during the given time, by steps of
//! one frame.
static void run(Simulation& simulation, double duration, float dt = 1.0f / 60.0f)
{
    if (!simulation.isRunning())
    {
        simulation.start();
        simulation.step(0.0f);
    }
    for (double t = 0.0; t < duration; t += double(dt))
        simulation.step(dt);
}

//------------------------------------------------------------------------------
TEST(TestSimulation, TimedEventGraphGivesExactDates)
{
    // Howard2: same dates with frames of 1/60 s and of 0.5 s.
    std::vector<std::vector<double>> dates[2];
    float const steps[2] = { 1.0f / 60.0f, 0.5f };
    for (size_t i = 0u; i < 2u; ++i)
    {
        Net net(TypeOfNet::TimedPetriNet);
        bool springify;
        ASSERT_STREQ(loadFromFile(net, "../data/examples/Howard2.json", springify).c_str(), "");
        Simulation simulation(net);
        run(simulation, 30.0, steps[i]);
        for (size_t t = 0u; t < net.transitions().size(); ++t)
        {
            auto d = simulation.firingDates(t);
            while ((!d.empty()) && (d.back() > 25.0)) // same horizon for both
                d.pop_back();
            dates[i].push_back(d);
        }
        ASSERT_GT(simulation.totalFirings(), 4u);
    }
    ASSERT_EQ(dates[0], dates[1]);
}

//------------------------------------------------------------------------------
TEST(TestSimulation, RestoresTheInitialMarking)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool springify;
    ASSERT_STREQ(loadFromFile(net, "../data/examples/InputsOutputs.json", springify).c_str(), "");
    Marking const initial = net.tokens();
    Simulation simulation(net);
    run(simulation, 7.3);
    ASSERT_NE(net.tokens(), initial); // shown during the simulation
    simulation.stop();
    simulation.step(0.0f);
    simulation.step(0.0f);
    ASSERT_FALSE(simulation.isRunning());
    ASSERT_EQ(net.tokens(), initial);
    ASSERT_GT(simulation.firingDates(0u).size(), 0u); // kept after the end
}

//------------------------------------------------------------------------------
TEST(TestSimulation, PetriNetFiresClickedTransitions)
{
    // P0 (1) -> T0 -> P1
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "", 1u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1);
    Simulation simulation(net);
    run(simulation, 1.0);
    ASSERT_EQ(net.tokens(), Marking({ 1u, 0u })); // nothing clicked

    t0.receptivity = true; // the click
    run(simulation, 0.05);
    ASSERT_EQ(p0.tokens, 0u);
    ASSERT_EQ(p1.tokens, 0u); // on its way
    ASSERT_EQ(simulation.flights().size(), 1u);
    ASSERT_FALSE(t0.receptivity);
    run(simulation, 0.3);
    ASSERT_EQ(p1.tokens, 1u);
    ASSERT_TRUE(simulation.flights().empty());
    ASSERT_TRUE(simulation.isRunning()); // waits for the next click
}

//------------------------------------------------------------------------------
TEST(TestSimulation, UserAddsTokens)
{
    // A timed net stopped for lack of tokens, restarted by a token added by
    // the user (keys + and -) before it stops.
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "", 0u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1, 2.0f);
    net.addArc(p1, t1);
    net.addArc(t1, p0, 2.0f);
    Simulation simulation(net);
    simulation.start();
    simulation.step(0.0f);
    p0.increment();
    run(simulation, 1.0);
    ASSERT_EQ(simulation.firingDates(0u).size(), 1u);
    ASSERT_EQ(p0.tokens, 0u);
    run(simulation, 10.0);
    ASSERT_TRUE(simulation.isRunning()); // the token goes round
    ASSERT_GE(simulation.firingDates(1u).size(), 2u);
}

//------------------------------------------------------------------------------
TEST(TestSimulation, TimedNetStopsWhenDeadlocked)
{
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "", 1u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1, 1.0f);
    Simulation simulation(net);
    run(simulation, 2.0);
    ASSERT_FALSE(simulation.isRunning());
}

//------------------------------------------------------------------------------
TEST(TestSimulation, SeedReplaysConflicts)
{
    // P0 (1 token) feeds T0 and T1: the seed decides, the same each time.
    auto winner = [](uint64_t seed) {
        Net net(TypeOfNet::TimedPetriNet);
        Place& p0 = net.addPlace(0u, "", 1u);
        net.addPlace(1u, "", 0u);
        net.addPlace(2u, "", 0u);
        Transition& t0 = net.addTransition(0u, "");
        Transition& t1 = net.addTransition(1u, "");
        net.addArc(p0, t0);
        net.addArc(t0, net.places()[1], 1.0f);
        net.addArc(p0, t1);
        net.addArc(t1, net.places()[2], 1.0f);
        Simulation simulation(net);
        Engine::Options options;
        options.seed = seed;
        simulation.options(options);
        run(simulation, 0.5);
        EXPECT_EQ(simulation.seed(), seed);
        return simulation.firingDates(0u).empty() ? 1 : 0;
    };
    for (uint64_t seed = 1u; seed < 10u; ++seed)
        ASSERT_EQ(winner(seed), winner(seed));
}
