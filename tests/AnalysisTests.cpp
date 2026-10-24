// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "Analysis/Analysis.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief Net of the given places (initial tokens) and transitions, then arcs
//! "P0>T0" or "T0>P1".
static void build(Net& net, std::vector<size_t> const& tokens, size_t const transitions,
                  std::vector<std::string> const& arcs)
{
    for (size_t i = 0u; i < tokens.size(); ++i)
        net.addPlace(tokens[i]);
    for (size_t i = 0u; i < transitions; ++i)
        net.addTransition();
    for (auto const& arc: arcs)
    {
        size_t const sep = arc.find('>');
        Node* from = net.findNode(arc.substr(0u, sep));
        Node* to = net.findNode(arc.substr(sep + 1u));
        ASSERT_TRUE((from != nullptr) && (to != nullptr)) << arc;
        ASSERT_STREQ(net.addArc(*from, *to).c_str(), "") << arc;
    }
}

//------------------------------------------------------------------------------
//! \brief A token going round P0 -> T0 -> P1 -> T1 -> P0.
TEST(TestAnalysis, Cycle)
{
    Net net(TypeOfNet::PetriNet);
    build(net, { 1u, 0u }, 2u, { "P0>T0", "T0>P1", "P1>T1", "T1>P0" });

    ReachabilityGraph const graph = reachabilityGraph(net);
    ASSERT_TRUE(graph.complete);
    ASSERT_EQ(graph.states.size(), 2u);
    ASSERT_EQ(graph.states[0], Marking({ 1u, 0u }));
    ASSERT_EQ(graph.states[1], Marking({ 0u, 1u }));
    ASSERT_EQ(graph.edges.size(), 2u);
    ASSERT_TRUE(graph.deadlocks.empty());

    Properties const prop = properties(net);
    ASSERT_TRUE(prop.complete);
    ASSERT_EQ(prop.states, 2u);
    ASSERT_FALSE(prop.deadlock);
    ASSERT_TRUE(prop.dead_transitions.empty());
    ASSERT_TRUE(prop.bounded);
    ASSERT_EQ(prop.bounds, std::vector<size_t>({ 1u, 1u }));
    ASSERT_TRUE(prop.live);
    ASSERT_TRUE(prop.not_live.empty());
    ASSERT_TRUE(prop.notes.empty());

    std::string error;
    ASSERT_EQ(pInvariants(net, error), std::vector<std::vector<int64_t>>({ { 1, 1 } }));
    ASSERT_STREQ(error.c_str(), "");
    ASSERT_EQ(tInvariants(net, error), std::vector<std::vector<int64_t>>({ { 1, 1 } }));
    ASSERT_STREQ(error.c_str(), "");
}

//------------------------------------------------------------------------------
//! \brief P0 -> T0 -> P1 -> T1 -> P2: dies after two firings; T2 needs P3
//! which is never marked.
TEST(TestAnalysis, DeadlockAndDeadTransition)
{
    Net net(TypeOfNet::PetriNet);
    build(net, { 1u, 0u, 0u, 0u }, 3u, { "P0>T0", "T0>P1", "P1>T1", "T1>P2", "P3>T2", "T2>P0" });

    Properties const prop = properties(net);
    ASSERT_TRUE(prop.complete);
    ASSERT_EQ(prop.states, 3u);
    ASSERT_TRUE(prop.deadlock);
    ASSERT_EQ(prop.deadlock_marking, Marking({ 0u, 0u, 1u, 0u }));
    ASSERT_EQ(prop.deadlock_path, std::vector<size_t>({ 0u, 1u }));
    ASSERT_EQ(prop.dead_transitions, std::vector<size_t>({ 2u }));
    ASSERT_FALSE(prop.live);
    ASSERT_EQ(prop.not_live, std::vector<size_t>({ 0u, 1u, 2u }));

    std::string error;
    ASSERT_EQ(pInvariants(net, error), std::vector<std::vector<int64_t>>({ { 1, 1, 1, 1 } }));
    ASSERT_TRUE(tInvariants(net, error).empty());
}

//------------------------------------------------------------------------------
//! \brief Mutual exclusion: two processes Idle -> Busy -> Idle sharing a
//! mutex. The mutex and the busy places are a P-invariant.
TEST(TestAnalysis, MutualExclusion)
{
    // P0 Idle1, P1 Busy1, P2 Mutex, P3 Idle2, P4 Busy2
    Net net(TypeOfNet::PetriNet);
    build(net, { 1u, 0u, 1u, 1u, 0u }, 4u,
          { "P0>T0", "P2>T0", "T0>P1", "P1>T1", "T1>P0", "T1>P2",
            "P3>T2", "P2>T2", "T2>P4", "P4>T3", "T3>P3", "T3>P2" });

    Properties const prop = properties(net);
    ASSERT_TRUE(prop.complete);
    ASSERT_EQ(prop.states, 3u);
    ASSERT_FALSE(prop.deadlock);
    ASSERT_TRUE(prop.live);
    ASSERT_TRUE(prop.bounded);
    ASSERT_EQ(prop.bounds, std::vector<size_t>({ 1u, 1u, 1u, 1u, 1u }));

    std::string error;
    auto pinv = pInvariants(net, error);
    std::sort(pinv.begin(), pinv.end());
    ASSERT_EQ(pinv, std::vector<std::vector<int64_t>>({
        { 0, 0, 0, 1, 1 }, { 0, 1, 1, 0, 1 }, { 1, 1, 0, 0, 0 } }));
    auto tinv = tInvariants(net, error);
    std::sort(tinv.begin(), tinv.end());
    ASSERT_EQ(tinv, std::vector<std::vector<int64_t>>({ { 0, 0, 1, 1 }, { 1, 1, 0, 0 } }));
}

//------------------------------------------------------------------------------
//! \brief The source transition T0 fills P0 without bound: the reachability
//! graph stops, Karp-Miller gives OMEGA.
TEST(TestAnalysis, Unbounded)
{
    Net net(TypeOfNet::PetriNet);
    build(net, { 0u, 0u }, 2u, { "T0>P0", "P0>T1", "T1>P1" });

    ReachabilityGraph const graph = reachabilityGraph(net, 50u);
    ASSERT_FALSE(graph.complete);
    ASSERT_EQ(graph.states.size(), 50u);

    CoverabilityGraph const cover = coverabilityGraph(net);
    ASSERT_TRUE(cover.complete);
    ASSERT_STREQ(cover.error.c_str(), "");
    ASSERT_EQ(cover.states[0], Marking({ 0u, 0u }));
    ASSERT_EQ(cover.states[1], Marking({ OMEGA, 0u }));
    ASSERT_EQ(cover.states.size(), 3u);

    Properties const prop = properties(net, 50u);
    ASSERT_FALSE(prop.complete);
    ASSERT_FALSE(prop.deadlock);
    ASSERT_FALSE(prop.bounded);
    ASSERT_EQ(prop.bounds, std::vector<size_t>({ OMEGA, OMEGA }));
    ASSERT_TRUE(prop.dead_transitions.empty());
    ASSERT_FALSE(prop.live);
    ASSERT_EQ(prop.notes.size(), 1u);
}

//------------------------------------------------------------------------------
//! \brief Weights: P0 (4) -(2)-> T0 -(1)-> P1. Two firings then a deadlock.
TEST(TestAnalysis, Weights)
{
    Net net(TypeOfNet::PetriNet);
    build(net, { 4u, 0u }, 1u, { "P0>T0", "T0>P1" });
    net.findArc(net.places()[0], net.transitions()[0])->weight = 2u;

    Properties const prop = properties(net);
    ASSERT_EQ(prop.states, 3u);
    ASSERT_TRUE(prop.deadlock);
    ASSERT_EQ(prop.deadlock_marking, Marking({ 0u, 2u }));
    ASSERT_EQ(prop.bounds, std::vector<size_t>({ 4u, 2u }));

    std::string error;
    ASSERT_EQ(pInvariants(net, error), std::vector<std::vector<int64_t>>({ { 1, 2 } }));
}

//------------------------------------------------------------------------------
//! \brief Inhibitor and reset arcs: the reachability graph follows them,
//! Karp-Miller and the invariants say they do not apply.
TEST(TestAnalysis, Extensions)
{
    Net net(TypeOfNet::PetriNet);
    bool springify;
    ASSERT_STREQ(loadFromFile(net, "data/Extensions.json", springify).c_str(), "");

    // Stock 3 -(2)-> Move -(3)-> Buffer (capacity 4): fires once, Trash reset.
    Properties const prop = properties(net);
    ASSERT_TRUE(prop.complete);
    ASSERT_EQ(prop.states, 2u);
    ASSERT_TRUE(prop.deadlock);
    ASSERT_EQ(prop.deadlock_marking, Marking({ 1u, 0u, 3u, 0u }));

    CoverabilityGraph const cover = coverabilityGraph(net);
    ASSERT_FALSE(cover.complete);
    ASSERT_STREQ(cover.error.c_str(), "Karp-Miller does not apply to nets with inhibitor arcs");

    std::string error;
    pInvariants(net, error);
    ASSERT_STRNE(error.c_str(), "");
}

//------------------------------------------------------------------------------
//! \brief Not live but without deadlock: T0 fires once to the cycle of T1
//! and T2, then never again.
TEST(TestAnalysis, NotLiveWithoutDeadlock)
{
    Net net(TypeOfNet::PetriNet);
    build(net, { 1u, 0u, 0u }, 3u, { "P0>T0", "T0>P1", "P1>T1", "T1>P2", "P2>T2", "T2>P1" });

    Properties const prop = properties(net);
    ASSERT_FALSE(prop.deadlock);
    ASSERT_TRUE(prop.dead_transitions.empty());
    ASSERT_FALSE(prop.live);
    ASSERT_EQ(prop.not_live, std::vector<size_t>({ 0u }));
}

//------------------------------------------------------------------------------
//! \brief The examples are analyzed without crash, the dining philosophers
//! (taking both forks at once) do not deadlock.
TEST(TestAnalysis, Examples)
{
    Net net;
    bool springify;
    ASSERT_STREQ(loadFromFile(net, "../examples/Petri-Nets/Philosophers/Philosophers.json", springify).c_str(), "");
    Properties const prop = properties(net);
    ASSERT_TRUE(prop.complete);
    ASSERT_FALSE(prop.deadlock);
    ASSERT_TRUE(prop.bounded);
    ASSERT_TRUE(prop.live);

    std::string const text = report(net);
    ASSERT_THAT(text, HasSubstr("Deadlock: none"));
    ASSERT_THAT(text, HasSubstr("Live: yes"));
    ASSERT_THAT(text, HasSubstr("P-invariants:"));
}
