// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#define protected public
#define private public
#  include "PetriNet/PetriNet.hpp"
#undef protected
#undef private

#include "MaxPlus/Algorithms.hpp"
#include <cmath>

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestNodeCreation)
{
    // Create a Place from the mother class Node
    Node n1(Node::Type::Place, 2u, "");
    ASSERT_EQ(n1.id, 2u);
    ASSERT_EQ(n1.type, Node::Type::Place);
    ASSERT_STREQ(n1.key.c_str(), "P2");
    ASSERT_STREQ(n1.caption.c_str(), "P2");

    // Create a Transition from the mother class Node
    Node n2(Node::Type::Transition, 42u, "hello");
    ASSERT_EQ(n2.id, 42u);
    ASSERT_EQ(n2.type, Node::Type::Transition);
    ASSERT_STREQ(n2.key.c_str(), "T42");
    ASSERT_STREQ(n2.caption.c_str(), "hello");

    // Check the operator!=()
    //Node n3(Node::Type::Place, 42u, "");
    //No more in the API
    //ASSERT_EQ(n1 != n2, true); // Transition vs Place + different id
    //ASSERT_EQ(n3 != n2, true); // Transition vs Place + same id
    //ASSERT_EQ(n1 != n2, true); // different id

    // Check the copy operator (FIXME is it really needed ?)
    n1 = n2;
    ASSERT_EQ(n1.id, 42u);
    ASSERT_EQ(n1.type, Node::Type::Transition);
    ASSERT_STREQ(n1.key.c_str(), "T42");
    ASSERT_STREQ(n1.caption.c_str(), "hello");

    // Check the copy constructor (FIXME is it really needed ?)
    Node n4(n1);
    ASSERT_EQ(n4.id, 42u);
    ASSERT_EQ(n4.type, Node::Type::Transition);
    ASSERT_STREQ(n4.key.c_str(), "T42");
    ASSERT_STREQ(n4.caption.c_str(), "hello");

    // Check the operator==()
    //n4.x = 5.0f; n4.caption = "foo";
    //ASSERT_EQ(n1 == n4, true);
    //ASSERT_EQ(n1 == n2, true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestPlaceCreation)
{
    // Check the default constructor
    Place p1(42u, "Hello", 12u);
    ASSERT_EQ(p1.id, 42u);
    ASSERT_EQ(p1.type, Node::Type::Place);
    ASSERT_EQ(p1.tokens, 12u);
    ASSERT_STREQ(p1.key.c_str(), "P42");
    ASSERT_STREQ(p1.caption.c_str(), "Hello");

    // Check the copy constructor
    Place p2(p1);
    ASSERT_EQ(p2.id, 42u);
    ASSERT_EQ(p2.type, Node::Type::Place);
    ASSERT_EQ(p2.tokens, 12u);
    ASSERT_STREQ(p2.key.c_str(), "P42");
    ASSERT_STREQ(p2.caption.c_str(), "Hello");

    // Check the operator!=() and operator==()
    //ASSERT_EQ(p1 == p2, true);
    //ASSERT_EQ(p1 != p2, false);

    // Check the copy operator
    Place p3(0u, "world", 0u);
    //ASSERT_EQ(p1 == p3, false);
    //ASSERT_EQ(p1 != p3, true);

    p3 = p1;
    ASSERT_EQ(p3.id, 42u);
    ASSERT_EQ(p3.type, Node::Type::Place);
    ASSERT_EQ(p3.tokens, 12u);
    ASSERT_STREQ(p3.key.c_str(), "P42");
    ASSERT_STREQ(p3.caption.c_str(), "Hello");

    // Check the operator!=() and operator==()
    //ASSERT_EQ(p1 == p3, true);
    //ASSERT_EQ(p1 != p3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTransitionCreation)
{
    // Check the default constructor
    Transition t1(42u, "Hello", false);
    ASSERT_EQ(t1.id, 42u);
    ASSERT_EQ(t1.type, Node::Type::Transition);
    ASSERT_EQ(t1.receptivity, false);
    ASSERT_STREQ(t1.key.c_str(), "T42");
    ASSERT_STREQ(t1.caption.c_str(), "Hello");

    // Check the copy constructor
    Transition t2(t1);
    ASSERT_EQ(t2.id, 42u);
    ASSERT_EQ(t2.type, Node::Type::Transition);
    ASSERT_EQ(t2.receptivity, false);
    ASSERT_STREQ(t2.key.c_str(), "T42");
    ASSERT_STREQ(t2.caption.c_str(), "Hello");

    // Check the operator!=() and operator==()
    //ASSERT_EQ(t1 == t2, true);
    //ASSERT_EQ(t1 != t2, false);

    // Check the copy operator
    Transition t3(0u, "world", false);
    //ASSERT_EQ(t1 == t3, false);
    //ASSERT_EQ(t1 != t3, true);
    t3 = t1;
    ASSERT_EQ(t3.id, 42u);
    ASSERT_EQ(t3.type, Node::Type::Transition);
    ASSERT_EQ(t3.receptivity, false);
    ASSERT_STREQ(t3.key.c_str(), "T42");
    ASSERT_STREQ(t3.caption.c_str(), "Hello");

    // Check the operator!=() and operator==()
    //ASSERT_EQ(t1 == t3, true);
    //ASSERT_EQ(t1 != t3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestArcCreation)
{
    Transition t1(42u, "", true);
    Place p1(43u, "", 13u);

    // Check the default constructor: Transition --> Place. Arcs only store
    // the references of their nodes.
    Arc a1(t1, p1, 10.0f);
    ASSERT_EQ(a1.duration, 10.0f);
    ASSERT_EQ(a1.from.id, 42u);
    ASSERT_EQ(a1.from.type, Node::Type::Transition);
    ASSERT_STREQ(a1.from.key().c_str(), "T42");
    ASSERT_EQ(a1.to.id, 43u);
    ASSERT_EQ(a1.to.type, Node::Type::Place);
    ASSERT_STREQ(a1.to.key().c_str(), "P43");
    ASSERT_EQ(a1.place(), p1.nodeId());
    ASSERT_EQ(a1.transition(), t1.nodeId());

    // Check the default constructor: Place --> Transition
    Arc a2(p1, t1, 15.0f);
    ASSERT_FALSE(a2.duration.has_value());
    ASSERT_EQ(a2.to.id, 42u);
    ASSERT_EQ(a2.to.type, Node::Type::Transition);
    ASSERT_STREQ(a2.to.key().c_str(), "T42");
    ASSERT_EQ(a2.from.id, 43u);
    ASSERT_EQ(a2.from.type, Node::Type::Place);
    ASSERT_STREQ(a2.from.key().c_str(), "P43");
    ASSERT_EQ(a2.place(), p1.nodeId());
    ASSERT_EQ(a2.transition(), t1.nodeId());

    // Check the copy constructor
    Arc a3(a1);
    ASSERT_EQ(a3.duration, 10.0f);
    ASSERT_EQ(a3.from, a1.from);
    ASSERT_EQ(a3.to, a1.to);

    // Check the copy operator
    Arc a4(p1, t1, 15.0f);
    a4 = a1;
    ASSERT_EQ(a4.duration, 10.0f);
    ASSERT_EQ(a4.from, a1.from);
    ASSERT_EQ(a4.to, a1.to);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestArcsOfNodes)
{
    // P0 -> T0 -> P1, P1 -> T1
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "", 0.0f, 0.0f, 2u);
    Place& p1 = net.addPlace(1u, "", 0.0f, 0.0f, 0u);
    Transition& t0 = net.addTransition(0u, "", 0.0f, 0.0f);
    Transition& t1 = net.addTransition(1u, "", 0.0f, 0.0f);
    Transition& t2 = net.addTransition(2u, "", 0.0f, 0.0f);
    ASSERT_TRUE(net.addArc(p0, t0).empty());
    ASSERT_TRUE(net.addArc(t0, p1, 3.0f).empty());
    ASSERT_TRUE(net.addArc(p1, t1).empty());

    ASSERT_EQ(net.arcsIn(p0).size(), 0u);
    ASSERT_EQ(net.arcsOut(p0).size(), 1u);
    ASSERT_EQ(net.arcsOut(p0)[0].to, t0.nodeId());
    ASSERT_EQ(net.arcsIn(t0).size(), 1u);
    ASSERT_EQ(net.arcsOut(t0).size(), 1u);
    ASSERT_EQ(net.arcsOut(t0).indices()[0], 1u);
    ASSERT_EQ(&net.placeOf(net.arcsOut(t0)[0]), &p1);
    ASSERT_EQ(&net.transitionOf(net.arcsOut(t0)[0]), &t0);
    ASSERT_EQ(&net.node(t1.nodeId()), &t1);
    ASSERT_EQ(net.indexOf(net.arcs()[2]), 2u);

    // Predicates on transitions
    ASSERT_EQ(net.isEnabled(t0), true);
    ASSERT_EQ(net.isEnabled(t1), false);
    ASSERT_EQ(net.isEnabled(t2), true); // no input place
    ASSERT_EQ(net.canFire(t0), false);  // Petri net: receptivity is false
    t0.receptivity = true;
    ASSERT_EQ(net.canFire(t0), true);
    ASSERT_EQ(net.maxTokensToConsume(t0), 1u);
    ASSERT_EQ(net.maxTokensToConsume(t1), 0u);
    ASSERT_EQ(net.isInput(t0), false);
    ASSERT_EQ(net.isState(t0), true);
    ASSERT_EQ(net.isOutput(t1), true);
    ASSERT_EQ(net.isInput(t2), false); // isolated

    // A copy has its own nodes: its arcs do not refer to the original.
    Net copy(net);
    ASSERT_EQ(&copy.placeOf(copy.arcsOut(copy.transitions()[0])[0]), &copy.places()[1]);
    copy.places()[1].tokens = 42u;
    ASSERT_EQ(p1.tokens, 0u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestToKey)
{
    ASSERT_STREQ(Place::to_str(42u).c_str(), "P42");
    ASSERT_STREQ(Transition::to_str(0u).c_str(), "T0");
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestToMode)
{
    ASSERT_STREQ(to_str(TypeOfNet::GRAFCET).c_str(), "GRAFCET");
    ASSERT_STREQ(to_str(TypeOfNet::PetriNet).c_str(), "Petri net");
    ASSERT_STREQ(to_str(TypeOfNet::TimedPetriNet).c_str(), "Timed Petri net");
    ASSERT_STREQ(to_str(TypeOfNet::TimedEventGraph).c_str(), "Timed event graph");
    ASSERT_STREQ(to_str(TypeOfNet(42)).c_str(), "Undefined type of net");
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, PetriNetConstructor)
{
    Net timed_net(TypeOfNet::TimedPetriNet);
    ASSERT_EQ(timed_net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(timed_net.m_type, TypeOfNet::TimedPetriNet);

    Net net(TypeOfNet::PetriNet);
    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);
    ASSERT_EQ(net.m_type, TypeOfNet::PetriNet);

    Net grafcet(TypeOfNet::GRAFCET);
    ASSERT_EQ(grafcet.type(), TypeOfNet::GRAFCET);
    ASSERT_EQ(grafcet.m_type, TypeOfNet::GRAFCET);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, PetriNetDummy)
{
    std::vector<size_t> erroneous_arcs;

    // Check the default constructor: dummy net
    Net net(TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.m_type, TypeOfNet::TimedPetriNet);

    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(&net.places(), &net.m_places);
    ASSERT_EQ(net.places().size(), 0u);

    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(&net.transitions(), &net.m_transitions);
    ASSERT_EQ(net.transitions().size(), 0u);

    ASSERT_EQ(net.m_arcs.size(), 0u);
    ASSERT_EQ(&net.arcs(), &net.m_arcs);
    ASSERT_EQ(net.arcs().size(), 0u);

    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);

    ASSERT_EQ(net.modified, false);
    ASSERT_STREQ(net.name.c_str(), to_str(net.type()).c_str());

    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.tokens().empty(), true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestIncrementId)
{
    // Identifiers are indices: given ones follow the number of nodes.
    Net net(TypeOfNet::TimedPetriNet);

    net.addPlace(0u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 1u);

    net.addPlace(1u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 2u);

    net.addPlace(3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 3u);
    ASSERT_EQ(net.m_places[2].id, 2u);
    ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");

    net.addTransition(0u, "", 3.5f, 4.0f);
    net.addTransition(3.5f, 4.0f);
    ASSERT_EQ(net.m_next_transition_id, 2u);
    ASSERT_EQ(net.m_transitions[1].id, 1u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestAddRemoveOperations)
{
    std::vector<size_t> erroneous_arcs;
    std::string error;

    // Check the default constructor: dummy net
    Net net(TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), false);
    ASSERT_STREQ(error.c_str(), "Empty Petri net is not an event graph");

    // Add Place 0: net = P0
    Place& p0 = net.addPlace(3.14f, 2.16f, 10u);
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(p0.id, 0u);
    ASSERT_STREQ(p0.key.c_str(), "P0");
    ASSERT_STREQ(p0.caption.c_str(), "P0");
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), false);
    ASSERT_STREQ(error.c_str(), "The Petri net is not an event graph. Because:\n  P0 has no output arc\n  P0 has no input arc\n");
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(net.findNode("P0"), &p0);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");

    // Add Transition 0: net = P0 T0
    Transition* t0 = &net.addTransition(3.14f, 2.16f);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(t0->id, 0u);
    ASSERT_STREQ(t0->key.c_str(), "T0");
    ASSERT_STREQ(t0->caption.c_str(), "T0");
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), false);
    ASSERT_STREQ(error.c_str(), "The Petri net is not an event graph. Because:\n  P0 has no output arc\n  P0 has no input arc\n");
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(net.findNode("T0"), t0);
    ASSERT_EQ(net.m_transitions.size(), 1u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");

    // Add Place 1: net = P0 T0 P1
    Place& p1 = net.addPlace(3.14f, 2.16f, 10u);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(p1.id, 1u);
    ASSERT_STREQ(p1.key.c_str(), "P1");
    ASSERT_STREQ(p1.caption.c_str(), "P1");
    ASSERT_EQ(net.findNode("P1"), &p1);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");

    // Add arcs: net = P0--T0--P1
    ASSERT_TRUE(net.addArc(p0, *t0).empty());
    ASSERT_TRUE(net.addArc(*t0, p1).empty());
    Arc* a1 = net.findArc(p0, *t0);
    ASSERT_NE(a1, nullptr);
    Arc* a2 = net.findArc(*t0, p1);
    ASSERT_NE(a2, nullptr);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 2u);
    ASSERT_EQ(&net.m_arcs[0], a1);
    ASSERT_EQ(&net.m_arcs[1], a2);

    // Remove T0: net = P0  P1
    net.removeNode(*t0);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // Add T0 back: net = P0 T0 P1
    t0 = &net.addTransition(3.14f, 2.16f);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(t0->id, 0u);
    ASSERT_STREQ(t0->key.c_str(), "T0");
    ASSERT_STREQ(t0->caption.c_str(), "T0");
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), false);
    ASSERT_STREQ(error.c_str(), "The Petri net is not an event graph. Because:\n  P0 has no output arc\n  P0 has no input arc\n");
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(net.findNode("T0"), t0);
    ASSERT_EQ(net.m_transitions.size(), 1u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");

    // Add arcs back: net = P0--T0--P1
    ASSERT_TRUE(net.addArc(p0, *t0).empty());
    ASSERT_TRUE(net.addArc(*t0, p1).empty());
    a1 = net.findArc(p0, *t0);
    ASSERT_NE(a1, nullptr);
    a2 = net.findArc(*t0, p1);
    ASSERT_NE(a2, nullptr);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 2u);
    ASSERT_EQ(&net.m_arcs[0], a1);
    ASSERT_EQ(&net.m_arcs[1], a2);

    // Remove arc P0--T0: net = P0 T0--P1
    ASSERT_EQ(net.removeArc(p0, *t0), true);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 1u);
    ASSERT_EQ(&net.m_arcs[0], a1); // a2 has been merged into a1
    ASSERT_STREQ(net.m_arcs[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.m_arcs[0].to.key().c_str(), "P1");
    ASSERT_STREQ(net.node(net.m_arcs[0].from).caption.c_str(), "T0");
    ASSERT_STREQ(net.node(net.m_arcs[0].to).caption.c_str(), "P1");

    // Remove P1: net = P0 T0
    net.removeNode(p1);
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_EQ(net.m_transitions.size(), 1u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");

    // Remove P0: net = T0
    net.removeNode(p0);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 1u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");

    // Remove T0: net
    net.removeNode(*t0);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.isEmpty(), true);

    // Try removing non existing arc
    ASSERT_EQ(net.findArc(p0, *t0), nullptr);
    ASSERT_EQ(net.removeArc(p0, *t0), false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestDecrTokens)
{
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "Hello", 3.14f, 2.16f, 10u);
    ASSERT_EQ(p0.tokens, 10u);
    p0.decrement(2u);
    ASSERT_EQ(p0.tokens, 8u);
    p0.decrement();
    ASSERT_EQ(p0.tokens, 7u);
    p0.decrement(8u);
    ASSERT_EQ(p0.tokens, 0u);
    p0.decrement(1u);
    ASSERT_EQ(p0.tokens, 0u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestIncrTokens)
{
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "Hello", 3.14f, 2.16f, 0u);
    ASSERT_EQ(p0.tokens, 0u);
    p0.increment(2u);
    ASSERT_EQ(p0.tokens, 2u);
    p0.increment();
    ASSERT_EQ(p0.tokens, 3u);

    // Convert to GRAFCET, check if max number of tokens (= 1) have been respected
    std::vector<size_t> erroneous_arcs;
    std::string error;
    ASSERT_EQ(convertTo(net, TypeOfNet::GRAFCET, error, erroneous_arcs), true);
    ASSERT_EQ(error.empty(), true);
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(p0.tokens, 1u);
    p0.increment();
    ASSERT_EQ(p0.tokens, 1u);

    ASSERT_EQ(convertTo(net, TypeOfNet::PetriNet, error, erroneous_arcs), true);
    ASSERT_EQ(error.empty(), true);
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(p0.tokens, 1u);
    p0.increment();
    ASSERT_EQ(p0.tokens, 2u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestCapacityIsPerNet)
{
    // A GRAFCET and a Petri net live in the same process: the 1-token limit
    // of the GRAFCET does not apply to the Petri net, created before or after.
    Net petri1(TypeOfNet::PetriNet);
    Net grafcet(TypeOfNet::GRAFCET);
    Net petri2(TypeOfNet::PetriNet);

    Place& p1 = petri1.addPlace(0.0f, 0.0f, 5u);
    Place& s0 = grafcet.addPlace(0.0f, 0.0f, 5u);
    Place& p2 = petri2.addPlace(0.0f, 0.0f, 5u);
    ASSERT_EQ(p1.tokens, 5u);
    ASSERT_EQ(s0.tokens, 1u);
    ASSERT_EQ(p2.tokens, 5u);

    p1.increment(2u);
    s0.increment(2u);
    ASSERT_EQ(p1.tokens, 7u);
    ASSERT_EQ(s0.tokens, 1u);

    ASSERT_EQ(petri2.tokens({ 42u }), true);
    ASSERT_EQ(grafcet.tokens({ 42u }), true);
    ASSERT_EQ(p2.tokens, 42u);
    ASSERT_EQ(s0.tokens, 1u);

    // A copy keeps the capacity of its places.
    Net copy(grafcet);
    copy.places()[0].increment(3u);
    ASSERT_EQ(copy.places()[0].tokens, 1u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestCaptionAfterRemove)
{
    // Add P0 and P1
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(0u, "Hello", 3.14f, 2.16f, 10u);
    net.addPlace(1u, "World", 1.0f, 1.5f, 42u);

    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "Hello");
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "World");

    // Remove P0 check P1 is now P0 but with its caption
    net.removeNode(p0);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "World");
    ASSERT_EQ(net.position(net.m_places[0]).x, 1.0f); // Attributes from P1
    ASSERT_EQ(net.position(net.m_places[0]).y, 1.5f);
    ASSERT_EQ(net.m_places[0].tokens, 42u);

    // Add P1. Check we have P0 P1
    net.addPlace(1u, "", 2.0f, 2.5f, 24u);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "World");
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");

    // Remove P0 check P1 is now P0 but without its caption
    net.removeNode(net.m_places[0]);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_EQ(net.position(net.m_places[0]).x, 2.0f); // Attributes from P1
    ASSERT_EQ(net.position(net.m_places[0]).y, 2.5f);
    ASSERT_EQ(net.m_places[0].tokens, 24u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestDoubleAdd)
{
    Net net(TypeOfNet::TimedPetriNet);

    Place& p1 = net.addPlace(0u, "", 3.5f, 4.0f, 45u);
    Transition& t1 = net.addTransition(0u, "", 3.5f, 4.0f);
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(net.m_next_transition_id, 1u);

    ASSERT_TRUE(net.addArc(t1, p1).empty());
    ASSERT_EQ(net.m_arcs.size(), 1u);

    // The same arc twice
    ASSERT_FALSE(net.addArc(t1, p1).empty());
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 1u);

    // The opposite arc
    ASSERT_TRUE(net.addArc(p1, t1).empty());
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(net.m_next_transition_id, 1u);
    ASSERT_EQ(net.m_arcs.size(), 2u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestInvalidAddArc)
{
    Net net(TypeOfNet::TimedPetriNet);

    Transition t1(42u, "", true);
    Transition t2(43u, "", false);
    Place p1(44u, "", 13u);
    Place p2(45u, "", 13u);

    // Bad arc: Transition --> transition
    ASSERT_FALSE(net.addArc(t1, t2).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_FALSE(net.addArc(t1, t1).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // Bad arc: Place --> Place
    ASSERT_FALSE(net.addArc(p1, p2).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_FALSE(net.addArc(p1, p1).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // Bad arc: double insertion
    ASSERT_FALSE(net.addArc(t1, p2).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_FALSE(net.addArc(p2, t1).empty());
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    Transition& t3 = net.addTransition(0u, "", 3.5f, 4.0f);
    ASSERT_FALSE(net.addArc(t3, p2).empty());
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadedNetTimedPetri)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/Howard2.json", stringify).c_str(), "");
    ASSERT_STREQ(net.error().c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedEventGraph);
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 4u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 4u);
    ASSERT_EQ(net.m_arcs.size(), 10u);

    // Check places
    ASSERT_EQ(net.m_places[0].id, 0u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_EQ(net.m_places[0].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[0].tokens, 2u);
    ASSERT_EQ(net.arcsIn(net.m_places[0]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[0]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[0])[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsIn(net.m_places[0])[0].to.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsOut(net.m_places[0])[0].from.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsOut(net.m_places[0])[0].to.key().c_str(), "T2");

    ASSERT_EQ(net.m_places[1].id, 1u);
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
    ASSERT_EQ(net.m_places[1].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[1].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[1]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[1]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[1])[0].from.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsIn(net.m_places[1])[0].to.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsOut(net.m_places[1])[0].from.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsOut(net.m_places[1])[0].to.key().c_str(), "T0");

    ASSERT_EQ(net.m_places[2].id, 2u);
    ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].caption.c_str(), "P2");
    ASSERT_EQ(net.m_places[2].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[2].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[2]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[2]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[2])[0].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsIn(net.m_places[2])[0].to.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsOut(net.m_places[2])[0].from.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsOut(net.m_places[2])[0].to.key().c_str(), "T1");

    ASSERT_EQ(net.m_places[3].id, 3u);
    ASSERT_STREQ(net.m_places[3].key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].caption.c_str(), "P3");
    ASSERT_EQ(net.m_places[3].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[3].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[3]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[3]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[3])[0].from.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsIn(net.m_places[3])[0].to.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsOut(net.m_places[3])[0].from.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsOut(net.m_places[3])[0].to.key().c_str(), "T0");

    ASSERT_EQ(net.m_places[4].id, 4u);
    ASSERT_STREQ(net.m_places[4].key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].caption.c_str(), "P4");
    ASSERT_EQ(net.m_places[4].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[4].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[4]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[4]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[4])[0].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsIn(net.m_places[4])[0].to.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsOut(net.m_places[4])[0].from.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsOut(net.m_places[4])[0].to.key().c_str(), "T3");

    // Check transitions
    ASSERT_EQ(net.m_transitions[0].id, 0u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");
    ASSERT_EQ(net.m_transitions[0].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[0]).size(), 2u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[0]).size(), 1u);
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[0].to.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[0].from.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[0].to.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[1].from.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[1].to.key().c_str(), "T0");

    ASSERT_EQ(net.m_transitions[1].id, 1u);
    ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "T1");
    ASSERT_EQ(net.m_transitions[1].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[1]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[1]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[1])[0].from.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[1])[0].to.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[1])[0].from.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[1])[0].to.key().c_str(), "P1");

    ASSERT_EQ(net.m_transitions[2].id, 2u);
    ASSERT_STREQ(net.m_transitions[2].key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].caption.c_str(), "T2");
    ASSERT_EQ(net.m_transitions[2].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[2]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[2]).size(), 2u);
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[0].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[0].to.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[1].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[1].to.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[0].from.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[0].to.key().c_str(), "T2");

    ASSERT_EQ(net.m_transitions[3].id, 3u);
    ASSERT_STREQ(net.m_transitions[3].key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].caption.c_str(), "T3");
    ASSERT_EQ(net.m_transitions[3].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[3]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[3]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[3])[0].from.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[3])[0].to.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[3])[0].from.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[3])[0].to.key().c_str(), "P3");

    // Check arcs
    ASSERT_EQ(net.m_arcs[0].from.id, 3u);
    ASSERT_EQ(net.m_arcs[0].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[0].from.key().c_str(), "P3");
    ASSERT_EQ(net.m_arcs[0].to.id, 0u);
    ASSERT_EQ(net.m_arcs[0].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[0].to.key().c_str(), "T0");
    ASSERT_FALSE(net.m_arcs[0].duration.has_value());

    ASSERT_EQ(net.m_arcs[1].from.id, 1u);
    ASSERT_EQ(net.m_arcs[1].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[1].from.key().c_str(), "P1");
    ASSERT_EQ(net.m_arcs[1].to.id, 0u);
    ASSERT_EQ(net.m_arcs[1].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[1].to.key().c_str(), "T0");
    ASSERT_FALSE(net.m_arcs[1].duration.has_value());

    ASSERT_EQ(net.m_arcs[2].from.id, 0u);
    ASSERT_EQ(net.m_arcs[2].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[2].from.key().c_str(), "T0");
    ASSERT_EQ(net.m_arcs[2].to.id, 0u);
    ASSERT_EQ(net.m_arcs[2].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[2].to.key().c_str(), "P0");
    ASSERT_EQ(net.m_arcs[2].duration, 5u);

    ASSERT_EQ(net.m_arcs[3].from.id, 0u);
    ASSERT_EQ(net.m_arcs[3].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[3].from.key().c_str(), "P0");
    ASSERT_EQ(net.m_arcs[3].to.id, 2u);
    ASSERT_EQ(net.m_arcs[3].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[3].to.key().c_str(), "T2");
    ASSERT_FALSE(net.m_arcs[3].duration.has_value());

    ASSERT_EQ(net.m_arcs[4].from.id, 2u);
    ASSERT_EQ(net.m_arcs[4].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[4].from.key().c_str(), "T2");
    ASSERT_EQ(net.m_arcs[4].to.id, 2u);
    ASSERT_EQ(net.m_arcs[4].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[4].to.key().c_str(), "P2");
    ASSERT_EQ(net.m_arcs[4].duration, 3u);

    ASSERT_EQ(net.m_arcs[5].from.id, 2u);
    ASSERT_EQ(net.m_arcs[5].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[5].from.key().c_str(), "P2");
    ASSERT_EQ(net.m_arcs[5].to.id, 1u);
    ASSERT_EQ(net.m_arcs[5].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[5].to.key().c_str(), "T1");
    ASSERT_FALSE(net.m_arcs[5].duration.has_value());

    ASSERT_EQ(net.m_arcs[6].from.id, 1u);
    ASSERT_EQ(net.m_arcs[6].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[6].from.key().c_str(), "T1");
    ASSERT_EQ(net.m_arcs[6].to.id, 1u);
    ASSERT_EQ(net.m_arcs[6].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[6].to.key().c_str(), "P1");
    ASSERT_EQ(net.m_arcs[6].duration, 5u);

    ASSERT_EQ(net.m_arcs[7].from.id, 2u);
    ASSERT_EQ(net.m_arcs[7].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[7].from.key().c_str(), "T2");
    ASSERT_EQ(net.m_arcs[7].to.id, 4u);
    ASSERT_EQ(net.m_arcs[7].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[7].to.key().c_str(), "P4");
    ASSERT_EQ(net.m_arcs[7].duration, 1u);

    ASSERT_EQ(net.m_arcs[8].from.id, 4u);
    ASSERT_EQ(net.m_arcs[8].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[8].from.key().c_str(), "P4");
    ASSERT_EQ(net.m_arcs[8].to.id, 3u);
    ASSERT_EQ(net.m_arcs[8].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[8].to.key().c_str(), "T3");
    ASSERT_FALSE(net.m_arcs[8].duration.has_value());

    ASSERT_EQ(net.m_arcs[9].from.id, 3u);
    ASSERT_EQ(net.m_arcs[9].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[9].from.key().c_str(), "T3");
    ASSERT_EQ(net.m_arcs[9].to.id, 3u);
    ASSERT_EQ(net.m_arcs[9].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[9].to.key().c_str(), "P3");
    ASSERT_EQ(net.m_arcs[9].duration, 1u);

    // Can we access to nodes ?
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.findNode("P0"), &net.m_places[0]);
    ASSERT_EQ(net.findNode("P1"), &net.m_places[1]);
    ASSERT_EQ(net.findNode("P2"), &net.m_places[2]);
    ASSERT_EQ(net.findNode("P3"), &net.m_places[3]);
    ASSERT_EQ(net.findNode("P4"), &net.m_places[4]);
    ASSERT_EQ(net.findNode("P5"), nullptr);
    ASSERT_EQ(net.findPlace(0u), &net.m_places[0]);
    ASSERT_EQ(net.findPlace(1u), &net.m_places[1]);
    ASSERT_EQ(net.findPlace(2u), &net.m_places[2]);
    ASSERT_EQ(net.findPlace(3u), &net.m_places[3]);
    ASSERT_EQ(net.findPlace(4u), &net.m_places[4]);
    ASSERT_EQ(net.findPlace(5u), nullptr);
    ASSERT_EQ(net.findNode("T0"), &net.m_transitions[0]);
    ASSERT_EQ(net.findNode("T1"), &net.m_transitions[1]);
    ASSERT_EQ(net.findNode("T2"), &net.m_transitions[2]);
    ASSERT_EQ(net.findNode("T3"), &net.m_transitions[3]);
    ASSERT_EQ(net.findNode("T4"), nullptr);
    ASSERT_EQ(net.findTransition(1u), &net.m_transitions[1]);
    ASSERT_EQ(net.findTransition(2u), &net.m_transitions[2]);
    ASSERT_EQ(net.findTransition(3u), &net.m_transitions[3]);
    ASSERT_EQ(net.findTransition(4u), nullptr);
    ASSERT_EQ(net.findNode("foo"), nullptr);
    ASSERT_EQ(net.findNode("T2o"), nullptr);
    ASSERT_EQ(net.findNode("P0uet"), nullptr);
    ASSERT_EQ(net.findNode(""), nullptr);

    // Can we access to arcs ?
    Arc* arc;

    arc = net.findArc(*net.findNode("P3"), *net.findNode("T0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P3");
    ASSERT_STREQ(arc->to.key().c_str(), "T0");
    ASSERT_FALSE(arc->duration.has_value());

    arc = net.findArc(*net.findNode("P1"), *net.findNode("T0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P1");
    ASSERT_STREQ(arc->to.key().c_str(), "T0");
    ASSERT_FALSE(arc->duration.has_value());

    arc = net.findArc(*net.findNode("T0"), *net.findNode("P0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T0");
    ASSERT_STREQ(arc->to.key().c_str(), "P0");
    ASSERT_EQ(arc->duration, 5u);

    arc = net.findArc(*net.findNode("P0"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P0");
    ASSERT_STREQ(arc->to.key().c_str(), "T2");
    ASSERT_FALSE(arc->duration.has_value());

    arc = net.findArc(*net.findNode("T2"), *net.findNode("P2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T2");
    ASSERT_STREQ(arc->to.key().c_str(), "P2");
    ASSERT_EQ(arc->duration, 3u);

    arc = net.findArc(*net.findNode("P2"), *net.findNode("T1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P2");
    ASSERT_STREQ(arc->to.key().c_str(), "T1");
    ASSERT_FALSE(arc->duration.has_value());

    arc = net.findArc(*net.findNode("T1"), *net.findNode("P1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T1");
    ASSERT_STREQ(arc->to.key().c_str(), "P1");
    ASSERT_EQ(arc->duration, 5u);

    arc = net.findArc(*net.findNode("T2"), *net.findNode("P4"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T2");
    ASSERT_STREQ(arc->to.key().c_str(), "P4");
    ASSERT_EQ(arc->duration, 1u);

    arc = net.findArc(*net.findNode("P4"), *net.findNode("T3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P4");
    ASSERT_STREQ(arc->to.key().c_str(), "T3");
    ASSERT_FALSE(arc->duration.has_value());

    arc = net.findArc(*net.findNode("T3"), *net.findNode("P3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T3");
    ASSERT_STREQ(arc->to.key().c_str(), "P3");
    ASSERT_EQ(arc->duration, 1u);

    // At least one token
    ASSERT_EQ(net.isEnabled(net.m_transitions[0]), false);
    ASSERT_EQ(net.isEnabled(net.m_transitions[1]), false);
    ASSERT_EQ(net.isEnabled(net.m_transitions[2]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[3]), false);

#if 0 // FIXMEEEEEEEEEE A FINALISER
    // Receptivity
    ASSERT_EQ(net.isEnabled(net.m_transitions[0]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[1]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[2]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[3]), true);

    // Can fire ? (Version 1)
    ASSERT_EQ(net.canFire(net.m_transitions[0]), true);
    ASSERT_EQ(net.canFire(net.m_transitions[1]), false);
    ASSERT_EQ(net.canFire(net.m_transitions[2]), false);
    ASSERT_EQ(net.canFire(net.m_transitions[3]), false);
#endif
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadedNetGraphEvent)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "data/EventGraph2.json", stringify).c_str(), "");
    ASSERT_STREQ(net.error().c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedEventGraph);
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 4u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 4u);
    ASSERT_EQ(net.m_arcs.size(), 10u);

    // Check places
    ASSERT_EQ(net.m_places[0].id, 0u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_EQ(net.m_places[0].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[0].tokens, 2u);
    ASSERT_EQ(net.arcsIn(net.m_places[0]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[0]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[0])[0].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsIn(net.m_places[0])[0].to.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsOut(net.m_places[0])[0].from.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsOut(net.m_places[0])[0].to.key().c_str(), "T0");

    ASSERT_EQ(net.m_places[1].id, 1u);
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
    ASSERT_EQ(net.m_places[1].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[1].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[1]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[1]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[1])[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsIn(net.m_places[1])[0].to.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsOut(net.m_places[1])[0].from.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsOut(net.m_places[1])[0].to.key().c_str(), "T1");

    ASSERT_EQ(net.m_places[2].id, 2u);
    ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].caption.c_str(), "P2");
    ASSERT_EQ(net.m_places[2].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[2].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[2]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[2]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[2])[0].from.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsIn(net.m_places[2])[0].to.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsOut(net.m_places[2])[0].from.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsOut(net.m_places[2])[0].to.key().c_str(), "T2");

    ASSERT_EQ(net.m_places[3].id, 3u);
    ASSERT_STREQ(net.m_places[3].key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].caption.c_str(), "P3");
    ASSERT_EQ(net.m_places[3].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[3].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[3]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[3]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[3])[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsIn(net.m_places[3])[0].to.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsOut(net.m_places[3])[0].from.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsOut(net.m_places[3])[0].to.key().c_str(), "T3");

    ASSERT_EQ(net.m_places[4].id, 4u);
    ASSERT_STREQ(net.m_places[4].key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].caption.c_str(), "P4");
    ASSERT_EQ(net.m_places[4].type, Node::Type::Place);
    ASSERT_EQ(net.m_places[4].tokens, 0u);
    ASSERT_EQ(net.arcsIn(net.m_places[4]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_places[4]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_places[4])[0].from.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsIn(net.m_places[4])[0].to.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsOut(net.m_places[4])[0].from.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsOut(net.m_places[4])[0].to.key().c_str(), "T2");

    // Check transitions
    ASSERT_EQ(net.m_transitions[0].id, 0u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");
    ASSERT_EQ(net.m_transitions[0].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[0]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[0]).size(), 2u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[0].from.key().c_str(), "P0");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[0])[0].to.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[0].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[0].to.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[1].from.key().c_str(), "T0");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[0])[1].to.key().c_str(), "P3");

    ASSERT_EQ(net.m_transitions[1].id, 1u);
    ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "T1");
    ASSERT_EQ(net.m_transitions[1].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[1]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[1]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[1])[0].from.key().c_str(), "P1");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[1])[0].to.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[1])[0].from.key().c_str(), "T1");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[1])[0].to.key().c_str(), "P2");

    ASSERT_EQ(net.m_transitions[2].id, 2u);
    ASSERT_STREQ(net.m_transitions[2].key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].caption.c_str(), "T2");
    ASSERT_EQ(net.m_transitions[2].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[2]).size(), 2u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[2]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[0].from.key().c_str(), "P2");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[0].to.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[1].from.key().c_str(), "P4");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[2])[1].to.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[0].from.key().c_str(), "T2");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[2])[0].to.key().c_str(), "P0");

    ASSERT_EQ(net.m_transitions[3].id, 3u);
    ASSERT_STREQ(net.m_transitions[3].key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].caption.c_str(), "T3");
    ASSERT_EQ(net.m_transitions[3].type, Node::Type::Transition);
    ASSERT_EQ(net.arcsIn(net.m_transitions[3]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[3]).size(), 1u);
    ASSERT_STREQ(net.arcsIn(net.m_transitions[3])[0].from.key().c_str(), "P3");
    ASSERT_STREQ(net.arcsIn(net.m_transitions[3])[0].to.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[3])[0].from.key().c_str(), "T3");
    ASSERT_STREQ(net.arcsOut(net.m_transitions[3])[0].to.key().c_str(), "P4");

    // Check arcs
    ASSERT_EQ(net.m_arcs[0].from.id, 0u);
    ASSERT_EQ(net.m_arcs[0].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[0].from.key().c_str(), "P0");
    ASSERT_EQ(net.m_arcs[0].to.id, 0u);
    ASSERT_EQ(net.m_arcs[0].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[0].to.key().c_str(), "T0");
    ASSERT_FALSE(net.m_arcs[0].duration.has_value());

    ASSERT_EQ(net.m_arcs[1].from.id, 0u);
    ASSERT_EQ(net.m_arcs[1].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[1].from.key().c_str(), "T0");
    ASSERT_EQ(net.m_arcs[1].to.id, 1u);
    ASSERT_EQ(net.m_arcs[1].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[1].to.key().c_str(), "P1");
    ASSERT_EQ(net.m_arcs[1].duration, 5u);

    ASSERT_EQ(net.m_arcs[2].from.id, 1u);
    ASSERT_EQ(net.m_arcs[2].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[2].from.key().c_str(), "P1");
    ASSERT_EQ(net.m_arcs[2].to.id, 1u);
    ASSERT_EQ(net.m_arcs[2].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[2].to.key().c_str(), "T1");
    ASSERT_FALSE(net.m_arcs[2].duration.has_value());

    ASSERT_EQ(net.m_arcs[3].from.id, 1u);
    ASSERT_EQ(net.m_arcs[3].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[3].from.key().c_str(), "T1");
    ASSERT_EQ(net.m_arcs[3].to.id, 2u);
    ASSERT_EQ(net.m_arcs[3].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[3].to.key().c_str(), "P2");
    ASSERT_EQ(net.m_arcs[3].duration, 3u);

    ASSERT_EQ(net.m_arcs[4].from.id, 2u);
    ASSERT_EQ(net.m_arcs[4].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[4].from.key().c_str(), "P2");
    ASSERT_EQ(net.m_arcs[4].to.id, 2u);
    ASSERT_EQ(net.m_arcs[4].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[4].to.key().c_str(), "T2");
    ASSERT_FALSE(net.m_arcs[4].duration.has_value());

    ASSERT_EQ(net.m_arcs[5].from.id, 2u);
    ASSERT_EQ(net.m_arcs[5].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[5].from.key().c_str(), "T2");
    ASSERT_EQ(net.m_arcs[5].to.id, 0u);
    ASSERT_EQ(net.m_arcs[5].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[5].to.key().c_str(), "P0");
    ASSERT_EQ(net.m_arcs[5].duration, 5u);

    ASSERT_EQ(net.m_arcs[6].from.id, 0u);
    ASSERT_EQ(net.m_arcs[6].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[6].from.key().c_str(), "T0");
    ASSERT_EQ(net.m_arcs[6].to.id, 3u);
    ASSERT_EQ(net.m_arcs[6].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[6].to.key().c_str(), "P3");
    ASSERT_EQ(net.m_arcs[6].duration, 1u);

    ASSERT_EQ(net.m_arcs[7].from.id, 3u);
    ASSERT_EQ(net.m_arcs[7].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[7].from.key().c_str(), "P3");
    ASSERT_EQ(net.m_arcs[7].to.id, 3u);
    ASSERT_EQ(net.m_arcs[7].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[7].to.key().c_str(), "T3");
    ASSERT_FALSE(net.m_arcs[7].duration.has_value());

    ASSERT_EQ(net.m_arcs[8].from.id, 3u);
    ASSERT_EQ(net.m_arcs[8].from.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[8].from.key().c_str(), "T3");
    ASSERT_EQ(net.m_arcs[8].to.id, 4u);
    ASSERT_EQ(net.m_arcs[8].to.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[8].to.key().c_str(), "P4");
    ASSERT_EQ(net.m_arcs[8].duration, 1u);

    ASSERT_EQ(net.m_arcs[9].from.id, 4u);
    ASSERT_EQ(net.m_arcs[9].from.type, Node::Type::Place);
    ASSERT_STREQ(net.m_arcs[9].from.key().c_str(), "P4");
    ASSERT_EQ(net.m_arcs[9].to.id, 2u);
    ASSERT_EQ(net.m_arcs[9].to.type, Node::Type::Transition);
    ASSERT_STREQ(net.m_arcs[9].to.key().c_str(), "T2");
    ASSERT_FALSE(net.m_arcs[9].duration.has_value());

    // Can we access to nodes ?
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.findNode("P0"), &net.m_places[0]);
    ASSERT_EQ(net.findNode("P1"), &net.m_places[1]);
    ASSERT_EQ(net.findNode("P2"), &net.m_places[2]);
    ASSERT_EQ(net.findNode("P3"), &net.m_places[3]);
    ASSERT_EQ(net.findNode("P4"), &net.m_places[4]);
    ASSERT_EQ(net.findNode("P5"), nullptr);
    ASSERT_EQ(net.findPlace(0u), &net.m_places[0]);
    ASSERT_EQ(net.findPlace(1u), &net.m_places[1]);
    ASSERT_EQ(net.findPlace(2u), &net.m_places[2]);
    ASSERT_EQ(net.findPlace(3u), &net.m_places[3]);
    ASSERT_EQ(net.findPlace(4u), &net.m_places[4]);
    ASSERT_EQ(net.findPlace(5u), nullptr);
    ASSERT_EQ(net.findNode("T0"), &net.m_transitions[0]);
    ASSERT_EQ(net.findNode("T1"), &net.m_transitions[1]);
    ASSERT_EQ(net.findNode("T2"), &net.m_transitions[2]);
    ASSERT_EQ(net.findNode("T3"), &net.m_transitions[3]);
    ASSERT_EQ(net.findNode("T4"), nullptr);
    ASSERT_EQ(net.findTransition(1u), &net.m_transitions[1]);
    ASSERT_EQ(net.findTransition(2u), &net.m_transitions[2]);
    ASSERT_EQ(net.findTransition(3u), &net.m_transitions[3]);
    ASSERT_EQ(net.findTransition(4u), nullptr);
    ASSERT_EQ(net.findNode("pouet"), nullptr);
    ASSERT_EQ(net.findNode(""), nullptr);

    // Can we access to arcs ?
    Arc* arc;

    arc = net.findArc(*net.findNode("T0"), *net.findNode("P1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T0");
    ASSERT_STREQ(arc->to.key().c_str(), "P1");

    arc = net.findArc(*net.findNode("T0"), *net.findNode("P3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T0");
    ASSERT_STREQ(arc->to.key().c_str(), "P3");

    arc = net.findArc(*net.findNode("T1"), *net.findNode("P2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T1");
    ASSERT_STREQ(arc->to.key().c_str(), "P2");

    arc = net.findArc(*net.findNode("T2"), *net.findNode("P0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T2");
    ASSERT_STREQ(arc->to.key().c_str(), "P0");

    arc = net.findArc(*net.findNode("T3"), *net.findNode("P4"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "T3");
    ASSERT_STREQ(arc->to.key().c_str(), "P4");

    arc = net.findArc(*net.findNode("P0"), *net.findNode("T0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P0");
    ASSERT_STREQ(arc->to.key().c_str(), "T0");

    arc = net.findArc(*net.findNode("P1"), *net.findNode("T1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P1");
    ASSERT_STREQ(arc->to.key().c_str(), "T1");

    arc = net.findArc(*net.findNode("P2"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P2");
    ASSERT_STREQ(arc->to.key().c_str(), "T2");

    arc = net.findArc(*net.findNode("P3"), *net.findNode("T3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P3");
    ASSERT_STREQ(arc->to.key().c_str(), "T3");

    arc = net.findArc(*net.findNode("P4"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key().c_str(), "P4");
    ASSERT_STREQ(arc->to.key().c_str(), "T2");

    // At least one token
    ASSERT_EQ(net.isEnabled(net.m_transitions[0]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[1]), false);
    ASSERT_EQ(net.isEnabled(net.m_transitions[2]), false);
    ASSERT_EQ(net.isEnabled(net.m_transitions[3]), false);

#if 0 // FIXMEEEEEEEEEEEEEEE
    // Receptivity
    ASSERT_EQ(net.isEnabled(net.m_transitions[0]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[1]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[2]), true);
    ASSERT_EQ(net.isEnabled(net.m_transitions[3]), true);

    // Can fire ? (Version 1)
    ASSERT_EQ(net.canFire(net.m_transitions[0]), true);
    ASSERT_EQ(net.canFire(net.m_transitions[1]), false);
    ASSERT_EQ(net.canFire(net.m_transitions[2]), false);
    ASSERT_EQ(net.canFire(net.m_transitions[3]), false);
#endif
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestRemoveNode)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/Howard2.json", stringify).c_str(), "");
    ASSERT_STREQ(net.error().c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedEventGraph);
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 4u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 4u);
    ASSERT_EQ(net.m_arcs.size(), 10u);

    // *** Delete Transition 0. Check that T3 is now T0 and places linking T0
    // have been removed.
    net.removeNode(*net.findNode("T0"));
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_EQ(net.m_next_transition_id, 3u);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_EQ(net.m_transitions.size(), 3u);
    ASSERT_EQ(net.m_arcs.size(), 4u);

    // Check Places are here
    ASSERT_NE(net.findPlace(0u), nullptr);
    ASSERT_NE(net.findPlace(1u), nullptr);
    ASSERT_EQ(net.findPlace(2u), nullptr);
    ASSERT_EQ(net.findPlace(3u), nullptr);
    ASSERT_EQ(net.findPlace(4u), nullptr);
    ASSERT_EQ(net.findPlace(5u), nullptr);

    // Check Transitions. Check T3 is no longer present while T0 has been removed
    ASSERT_NE(net.findTransition(0u), nullptr);
    ASSERT_NE(net.findTransition(1u), nullptr);
    ASSERT_NE(net.findTransition(2u), nullptr);
    ASSERT_EQ(net.findTransition(3u), nullptr);

    // Check arcs from/to previous T0 no longer exist.
    // Check arcs from/to previous T3 (now T0) exist.
    ASSERT_EQ(net.findArc(*net.findTransition(0u), *net.findPlace(0u)), nullptr);
    ASSERT_EQ(net.findArc(*net.findTransition(1u), *net.findPlace(1u)), nullptr);
    ASSERT_EQ(net.findArc(*net.findPlace(0u), *net.findTransition(2u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(1u), *net.findTransition(0u)), nullptr);
    ASSERT_NE(net.findArc(*net.findTransition(2u), *net.findPlace(0u)), nullptr);
    ASSERT_NE(net.findArc(*net.findTransition(2u), *net.findPlace(1u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(0u), *net.findTransition(1u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(1u), *net.findTransition(0u)), nullptr);

    // In/out arcs Transition
    // T0 (previously T3)
    {
        auto const arcsIn = net.arcsIn(*net.findTransition(0u));
        auto const arcsOut = net.arcsOut(*net.findTransition(0u));
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0].from.key().c_str(), "P1");
        ASSERT_STREQ(arcsIn[0].to.key().c_str(), "T0");
    }
    // T1
    {
        auto const arcsIn = net.arcsIn(*net.findTransition(1u));
        auto const arcsOut = net.arcsOut(*net.findTransition(1u));
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0].from.key().c_str(), "P0");
        ASSERT_STREQ(arcsIn[0].to.key().c_str(), "T1");
    }
    // T2
    {
        auto const arcsIn = net.arcsIn(*net.findTransition(2u));
        auto const arcsOut = net.arcsOut(*net.findTransition(2u));
        ASSERT_EQ(arcsIn.size(), 0u);
        // Removing arcs keeps the order of the other ones.
        ASSERT_EQ(arcsOut.size(), 2u);
        ASSERT_STREQ(arcsOut[0].from.key().c_str(), "T2");
        ASSERT_STREQ(arcsOut[0].to.key().c_str(), "P0");
        ASSERT_EQ(arcsOut[0].duration, 3.0f);
        ASSERT_STREQ(arcsOut[1].from.key().c_str(), "T2");
        ASSERT_STREQ(arcsOut[1].to.key().c_str(), "P1");
        ASSERT_EQ(arcsOut[1].duration, 1.0f);
    }

    // In/out arcs Place
    // P0
    {
        auto const arcsIn = net.arcsIn(*net.findPlace(0u));
        auto const arcsOut = net.arcsOut(*net.findPlace(0u));
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0].from.key().c_str(), "P0");
        ASSERT_STREQ(arcsOut[0].to.key().c_str(), "T1");
        ASSERT_STREQ(arcsIn[0].from.key().c_str(), "T2");
        ASSERT_STREQ(arcsIn[0].to.key().c_str(), "P0");
    }
    // P1
    {
        auto const arcsIn = net.arcsIn(*net.findPlace(1u));
        auto const arcsOut = net.arcsOut(*net.findPlace(1u));
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0].from.key().c_str(), "P1");
        ASSERT_STREQ(arcsOut[0].to.key().c_str(), "T0");
        ASSERT_STREQ(arcsIn[0].from.key().c_str(), "T2");
        ASSERT_STREQ(arcsIn[0].to.key().c_str(), "P1");
    }

    // *** Delete Transition 2
    net.removeNode(*net.findNode("T2"));
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 2u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 2u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // T0
    {
        auto const arcsIn = net.arcsIn(*net.findTransition(0u));
        auto const arcsOut = net.arcsOut(*net.findTransition(0u));
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 0u);
    }
    // T1
    {
        auto const arcsIn = net.arcsIn(*net.findTransition(1u));
        auto const arcsOut = net.arcsOut(*net.findTransition(1u));
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 0u);
    }
}

//------------------------------------------------------------------------------
// Removing a node: identifiers stay 0 .. n-1, the last node of the same type
// takes the identifier of the removed one, arcs follow their nodes.
TEST(TestPetriNet, TestRemoveNodeRenumbers)
{
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(0u, "A", 0.0f, 0.0f, 1u);
    net.addPlace(1u, "", 10.0f, 0.0f, 2u);
    net.addPlace(2u, "C", 20.0f, 0.0f, 3u);
    Transition& t0 = net.addTransition(0u, "", 5.0f, 0.0f);
    ASSERT_TRUE(net.addArc(net.m_places[1], t0).empty());
    ASSERT_TRUE(net.addArc(t0, net.m_places[2], 4.0f).empty());
    ASSERT_TRUE(net.addArc(p0, t0).empty());

    // Remove P0: P2 becomes P0 (caption kept), P1 does not move.
    net.removeNode(p0);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_EQ(net.m_next_place_id, 2u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "C");
    ASSERT_EQ(net.m_places[0].tokens, 3u);
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
    ASSERT_EQ(net.m_places[1].tokens, 2u);

    // The arc to P0 is gone, the arc to P2 now goes to P0.
    ASSERT_EQ(net.m_arcs.size(), 2u);
    ASSERT_NE(net.findArc(net.m_places[1], net.m_transitions[0]), nullptr);
    Arc const* arc = net.findArc(net.m_transitions[0], net.m_places[0]);
    ASSERT_NE(arc, nullptr);
    ASSERT_EQ(arc->duration, 4.0f);
    ASSERT_EQ(net.arcsIn(net.m_transitions[0]).size(), 1u);
    ASSERT_EQ(net.arcsOut(net.m_transitions[0]).size(), 1u);
    ASSERT_EQ(&net.placeOf(net.arcsOut(net.m_transitions[0])[0]), &net.m_places[0]);

    // New nodes take the next identifier.
    ASSERT_STREQ(net.addPlace(0.0f, 0.0f).key.c_str(), "P2");
}

//------------------------------------------------------------------------------
// GRAFCET: what reads a step by its number follows its new number, what read
// the removed step reads a step that does not exist.
TEST(TestPetriNet, TestRemoveStepRenamesReferences)
{
    Net net(TypeOfNet::GRAFCET);
    net.addPlace(0u, "", 0.0f, 0.0f, 1u);
    net.addPlace(1u, "", 0.0f, 0.0f, 0u);
    net.addPlace(2u, "", 0.0f, 0.0f, 0u);
    net.addTransition(0u, "X2 . a", 0.0f, 0.0f);
    net.addTransition(1u, "X1", 0.0f, 0.0f);

    net.removeNode(net.m_places[1]);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "X1 . a");
    ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "X3");
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestMaxTokensToConsume)
{
    std::vector<size_t> erroneous_arcs;
    std::string error;

    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/EventGraph.json", stringify).c_str(), "");
    ASSERT_STREQ(net.error().c_str(), "");
    ASSERT_EQ(convertTo(net, TypeOfNet::PetriNet, error, erroneous_arcs), true);

    ASSERT_EQ(net.m_transitions.size(), 4u);
    //ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[0]), 1u); // u
    ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[1]), 0u); // x1
    ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[2]), 0u); // x2
    ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[3]), 0u); // y

    // TODO ASSERT_EQ(net.m_transitions[0].fire());
    //ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[0]), 1u); // u
    //ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[1]), 2u); // x1
    //ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[2]), 2u); // x2
    //ASSERT_EQ(net.maxTokensToConsume(net.m_transitions[3]), 0u); // y
}
