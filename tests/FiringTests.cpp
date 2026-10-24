// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Firing.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "MaxPlus/Algorithms.hpp"

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief P0 (2 tokens) -> T0 -> P1, P1 -(weight 2)-> T1 -> P0.
static Net cycle()
{
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "", 2u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1);
    net.addArc(p1, t1);
    net.addArc(t1, p0);
    net.findArc(p1, t1)->weight = 2u;
    return net;
}

//------------------------------------------------------------------------------
TEST(TestFiring, FireDoesNotModifyTheNet)
{
    Net const net = cycle();
    Marking m0 = net.tokens();
    ASSERT_EQ(m0, Marking({ 2u, 0u }));
    ASSERT_EQ(enabledTransitions(net, m0), std::vector<size_t>({ 0u }));

    Marking const m1 = fire(net, m0, 0u);
    ASSERT_EQ(m1, Marking({ 1u, 1u }));
    ASSERT_EQ(enabledTransitions(net, m1), std::vector<size_t>({ 0u }));
    Marking const m2 = fire(net, m1, 0u);
    ASSERT_EQ(m2, Marking({ 0u, 2u }));
    ASSERT_EQ(enabledTransitions(net, m2), std::vector<size_t>({ 1u }));
    ASSERT_EQ(fire(net, m2, 1u), Marking({ 1u, 0u }));

    // The net keeps its initial marking.
    ASSERT_EQ(net.tokens(), m0);
}

//------------------------------------------------------------------------------
TEST(TestFiring, ConsumeThenProduce)
{
    Net const net = cycle();
    Marking m = net.tokens();
    consume(net, m, 0u);
    ASSERT_EQ(m, Marking({ 1u, 0u })); // the token is on its way
    produce(net, m, 0u);
    ASSERT_EQ(m, Marking({ 1u, 1u }));
}

//------------------------------------------------------------------------------
TEST(TestFiring, CapacityCountsIncomingTokens)
{
    Net net = cycle();
    net.places()[1].capacity = 1u;
    Marking const m = net.tokens(); // P1 empty
    ASSERT_TRUE(isEnabled(net, m, 0u));
    Marking const incoming = { 0u, 1u }; // a token is going to P1
    ASSERT_FALSE(isEnabled(net, m, 0u, &incoming));
}

//------------------------------------------------------------------------------
TEST(TestFiring, IncidenceMatrices)
{
    Net const net = cycle();
    SparseMatrix<double> pre, post;
    incidenceMatrices(net, pre, post);
    ASSERT_EQ(pre.get(0u, 0u), 1.0);
    ASSERT_EQ(pre.get(1u, 1u), 2.0);
    ASSERT_EQ(post.get(1u, 0u), 1.0);
    ASSERT_EQ(post.get(0u, 1u), 1.0);
    ASSERT_EQ(pre.get(0u, 1u), 0.0);
}
