// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Firing.hpp"
#include "PetriNet/ColoredMarking.hpp"
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

//------------------------------------------------------------------------------
//! \brief P0 -(Success)-> T0 -> P1, P0 -(Failed)-> T1 -(Failed)-> P1.
static Net colored()
{
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "", 0u);
    Place& p1 = net.addPlace(1u, "", 0u);
    Transition& t0 = net.addTransition(0u, "");
    Transition& t1 = net.addTransition(1u, "");
    net.addArc(p0, t0);
    net.addArc(t0, p1);
    net.addArc(p0, t1);
    net.addArc(t1, p1);
    net.findArc(p0, t0)->color = Success;
    net.findArc(p0, t1)->color = Failed;
    net.findArc(t1, p1)->color = Failed;
    return net;
}

//------------------------------------------------------------------------------
TEST(TestFiring, ColoredArcsTakeTheirColor)
{
    Net const net = colored();
    ColoredMarking m(net.places().size());
    m.add(0u, Success);
    ASSERT_TRUE(isEnabled(net, m, 0u));
    ASSERT_FALSE(isEnabled(net, m, 1u));

    m.clear(0u);
    m.add(0u, Failed);
    ASSERT_FALSE(isEnabled(net, m, 0u));
    ASSERT_TRUE(isEnabled(net, m, 1u));

    // The uncolored rule sees one token: both are enabled.
    ASSERT_EQ(enabledTransitions(net, m.counts()), std::vector<size_t>({ 0u, 1u }));
}

//------------------------------------------------------------------------------
TEST(TestFiring, ColoredConsumeThenProduce)
{
    Net const net = colored();
    ColoredMarking m(net.places().size());
    m.add(0u, Success);
    m.add(0u, Failed);

    // Output arc without color: the result of the transition.
    consume(net, m, 0u);
    ASSERT_EQ(m.count(0u, Success), 0u);
    ASSERT_EQ(m.count(0u, Failed), 1u);
    produce(net, m, 0u, Canceled);
    ASSERT_EQ(m.count(1u, Canceled), 1u);

    // Output arc with a color: its color, whatever the result.
    consume(net, m, 1u);
    produce(net, m, 1u, Success);
    ASSERT_EQ(m.count(0u), 0u);
    ASSERT_EQ(m.count(1u, Failed), 1u);
    ASSERT_EQ(m.counts(), Marking({ 0u, 2u }));
}

//------------------------------------------------------------------------------
TEST(TestFiring, ColoredInhibitorAndReset)
{
    Net net = colored();
    net.findArc(net.places()[0], net.transitions()[0])->kind = ArcKind::Inhibitor;
    net.findArc(net.places()[0], net.transitions()[1])->kind = ArcKind::Reset;
    ColoredMarking m(net.places().size());
    m.add(0u, Failed, 2u);

    // The inhibitor arc only sees the Success tokens.
    ASSERT_TRUE(isEnabled(net, m, 0u));
    m.add(0u, Success);
    ASSERT_FALSE(isEnabled(net, m, 0u));

    // The reset arc only empties the Failed tokens.
    consume(net, m, 1u);
    ASSERT_EQ(m.count(0u, Failed), 0u);
    ASSERT_EQ(m.count(0u, Success), 1u);
}

//------------------------------------------------------------------------------
TEST(TestFiring, ColoredCapacityCountsAllColors)
{
    Net net = colored();
    net.places()[1].capacity = 1u;
    ColoredMarking m(net.places().size());
    m.add(0u, Success);
    ASSERT_TRUE(isEnabled(net, m, 0u));
    m.add(1u, Failed);
    ASSERT_FALSE(isEnabled(net, m, 0u));
    m.clear(1u);
    Marking const incoming = { 0u, 1u };
    ASSERT_FALSE(isEnabled(net, m, 0u, &incoming));
}

//------------------------------------------------------------------------------
//! \brief The places of the net hold colored tokens (Place::colors): the
//! editor draws the enabled transitions with them.
TEST(TestFiring, ColoredTokensOfThePlaces)
{
    Net net = colored();
    Place& p0 = net.places()[0];
    p0.tokens = 2u;
    p0.colors = { { Failed, 1u } };
    ASSERT_EQ(placeTokens(p0, AnyColor), 2u);
    ASSERT_EQ(placeTokens(p0, Failed), 1u);
    ASSERT_EQ(placeTokens(p0, Success), 1u); // not listed: Success
    ASSERT_TRUE(isEnabled(net, net.transitions()[0]));
    ASSERT_TRUE(isEnabled(net, net.transitions()[1]));

    p0.colors = { { Failed, 5u } }; // more than tokens: capped
    ASSERT_EQ(placeTokens(p0, Failed), 2u);
    ASSERT_FALSE(isEnabled(net, net.transitions()[0]));
    ASSERT_TRUE(isEnabled(net, net.transitions()[1]));
}

//------------------------------------------------------------------------------
//! \brief The user adds and removes tokens of a color (editor).
TEST(TestFiring, AddAndRemoveColoredTokens)
{
    Place place(0u, "", 1u); // one Success token
    place.increment(2u, Failed);
    place.increment(1u, Canceled);
    ASSERT_EQ(place.tokens, 4u);
    ASSERT_EQ(placeColors(place), (std::vector<std::pair<Color, size_t>>{
                                      { Success, 1u }, { Failed, 2u }, { Canceled, 1u } }));

    place.decrement(1u, Failed);
    ASSERT_EQ(placeTokens(place, Failed), 1u);
    place.decrement(5u, Failed); // only the ones there are
    ASSERT_EQ(placeTokens(place, Failed), 0u);
    ASSERT_EQ(place.tokens, 2u);

    // Any color: the Success ones first, then the last colors listed.
    place.decrement(1u);
    ASSERT_EQ(placeColors(place), (std::vector<std::pair<Color, size_t>>{ { Canceled, 1u } }));
    place.decrement(1u);
    ASSERT_EQ(place.tokens, 0u);
    ASSERT_TRUE(place.colors.empty());

    // Capacity.
    place.capacity = 2u;
    place.increment(5u, Failed);
    ASSERT_EQ(placeTokens(place, Failed), 2u);
}
