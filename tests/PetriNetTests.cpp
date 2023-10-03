//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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
#define protected public
#define private public
#  include "src/PetriNet.hpp"
#undef protected
#undef private

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestNodeCreation)
{
    // Create a Place from the mother class Node
    Node n1(Node::Place, 2u, "", 3.5f, 4.0f);
    ASSERT_EQ(n1.id, 2u);
    ASSERT_EQ(n1.type, Node::Place);
    ASSERT_EQ(n1.x, 3.5f);
    ASSERT_EQ(n1.y, 4.0f);
    ASSERT_STREQ(n1.key.c_str(), "P2");
    ASSERT_STREQ(n1.caption.c_str(), "P2");
    ASSERT_EQ(n1.arcsIn.size(), 0u);
    ASSERT_EQ(n1.arcsOut.size(), 0u);

    // Create a Transition from the mother class Node
    Node n2(Node::Transition, 42u, "hello", 4.0f, 3.5f);
    ASSERT_EQ(n2.id, 42u);
    ASSERT_EQ(n2.type, Node::Transition);
    ASSERT_EQ(n2.x, 4.0f);
    ASSERT_EQ(n2.y, 3.5f);
    ASSERT_STREQ(n2.key.c_str(), "T42");
    ASSERT_STREQ(n2.caption.c_str(), "hello");
    ASSERT_EQ(n2.arcsIn.size(), 0u);
    ASSERT_EQ(n2.arcsOut.size(), 0u);

    // Check the operator!=()
    Node n3(Node::Place, 42u, "", 4.0f, 3.5f);
    ASSERT_EQ(n1 != n2, true); // Transition vs Place + different id
    ASSERT_EQ(n3 != n2, true); // Transition vs Place + same id
    ASSERT_EQ(n1 != n2, true); // different id

    // Check the copy operator (FIXME is it really needed ?)
    n1 = n2;
    ASSERT_EQ(n1.id, 42u);
    ASSERT_EQ(n1.type, Node::Transition);
    ASSERT_EQ(n1.x, 4.0f);
    ASSERT_EQ(n1.y, 3.5f);
    ASSERT_STREQ(n1.key.c_str(), "T42");
    ASSERT_STREQ(n1.caption.c_str(), "hello");
    ASSERT_EQ(n1.arcsIn.size(), 0u);
    ASSERT_EQ(n1.arcsOut.size(), 0u);

    // Check the copy constructor (FIXME is it really needed ?)
    Node n4(n1);
    ASSERT_EQ(n4.id, 42u);
    ASSERT_EQ(n4.type, Node::Transition);
    ASSERT_EQ(n4.x, 4.0f);
    ASSERT_EQ(n4.y, 3.5f);
    ASSERT_STREQ(n4.key.c_str(), "T42");
    ASSERT_STREQ(n4.caption.c_str(), "hello");
    ASSERT_EQ(n4.arcsIn.size(), 0u);
    ASSERT_EQ(n4.arcsOut.size(), 0u);

    // Check the operator==()
    n4.x = 5.0f; n4.caption = "foo";
    ASSERT_EQ(n1 == n4, true);
    ASSERT_EQ(n1 == n2, true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestPlaceCreation)
{
    // Check the default constructor
    Place p1(42u, "Hello", 3.5f, 4.0f, 12u);
    ASSERT_EQ(p1.id, 42u);
    ASSERT_EQ(p1.type, Node::Place);
    ASSERT_EQ(p1.tokens, 12u);
    ASSERT_EQ(p1.x, 3.5f);
    ASSERT_EQ(p1.y, 4.0f);
    ASSERT_STREQ(p1.key.c_str(), "P42");
    ASSERT_STREQ(p1.caption.c_str(), "Hello");
    ASSERT_EQ(p1.arcsIn.size(), 0u);
    ASSERT_EQ(p1.arcsOut.size(), 0u);

    // Check the copy constructor
    Place p2(p1);
    ASSERT_EQ(p2.id, 42u);
    ASSERT_EQ(p2.type, Node::Place);
    ASSERT_EQ(p2.tokens, 12u);
    ASSERT_EQ(p2.x, 3.5f);
    ASSERT_EQ(p2.y, 4.0f);
    ASSERT_STREQ(p2.key.c_str(), "P42");
    ASSERT_STREQ(p2.caption.c_str(), "Hello");
    ASSERT_EQ(p2.arcsIn.size(), 0u);
    ASSERT_EQ(p2.arcsOut.size(), 0u);

    // Check the operator!=() and operator==()
    ASSERT_EQ(p1 == p2, true);
    ASSERT_EQ(p1 != p2, false);

    // Check the copy operator
    Place p3(0u, "world", 0.0f, 0.0f, 0u);
    ASSERT_EQ(p1 == p3, false);
    ASSERT_EQ(p1 != p3, true);
    p3 = p1;
    ASSERT_EQ(p3.id, 42u);
    ASSERT_EQ(p3.type, Node::Place);
    ASSERT_EQ(p3.tokens, 12u);
    ASSERT_EQ(p3.x, 3.5f);
    ASSERT_EQ(p3.y, 4.0f);
    ASSERT_STREQ(p3.key.c_str(), "P42");
    ASSERT_STREQ(p3.caption.c_str(), "Hello");
    ASSERT_EQ(p3.arcsIn.size(), 0u);
    ASSERT_EQ(p3.arcsOut.size(), 0u);

    // Check the operator!=() and operator==()
    ASSERT_EQ(p1 == p3, true);
    ASSERT_EQ(p1 != p3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTransitionCreation)
{
    // Check the default constructor
    Transition t1(42u, "Hello", 3.5f, 4.0f, 45u, false);
    ASSERT_EQ(t1.id, 42u);
    ASSERT_EQ(t1.type, Node::Transition);
    ASSERT_EQ(t1.angle, 45);
    ASSERT_EQ(t1.x, 3.5f);
    ASSERT_EQ(t1.y, 4.0f);
    ASSERT_EQ(t1.receptivity, false);
    ASSERT_STREQ(t1.key.c_str(), "T42");
    ASSERT_STREQ(t1.caption.c_str(), "Hello");
    ASSERT_EQ(t1.arcsIn.size(), 0u);
    ASSERT_EQ(t1.arcsOut.size(), 0u);
    ASSERT_EQ(t1.canFire(), false);
    ASSERT_EQ(t1.isInput(), false);
    ASSERT_EQ(t1.isOutput(), false);
    ASSERT_EQ(t1.isState(), false);

    // Check the copy constructor
    Transition t2(t1);
    ASSERT_EQ(t2.id, 42u);
    ASSERT_EQ(t2.type, Node::Transition);
    ASSERT_EQ(t2.angle, 45);
    ASSERT_EQ(t2.x, 3.5f);
    ASSERT_EQ(t2.y, 4.0f);
    ASSERT_EQ(t2.receptivity, false);
    ASSERT_STREQ(t2.key.c_str(), "T42");
    ASSERT_STREQ(t2.caption.c_str(), "Hello");
    ASSERT_EQ(t2.arcsIn.size(), 0u);
    ASSERT_EQ(t2.arcsOut.size(), 0u);
    ASSERT_EQ(t2.canFire(), false);
    ASSERT_EQ(t2.isInput(), false);
    ASSERT_EQ(t2.isOutput(), false);
    ASSERT_EQ(t2.isState(), false);

    // Check the operator!=() and operator==()
    ASSERT_EQ(t1 == t2, true);
    ASSERT_EQ(t1 != t2, false);

    // Check the copy operator
    Transition t3(0u, "world", 0.0f, 0.0f, 0u, false);
    ASSERT_EQ(t1 == t3, false);
    ASSERT_EQ(t1 != t3, true);
    t3 = t1;
    ASSERT_EQ(t3.id, 42u);
    ASSERT_EQ(t3.type, Node::Transition);
    ASSERT_EQ(t3.angle, 45);
    ASSERT_EQ(t3.x, 3.5f);
    ASSERT_EQ(t3.y, 4.0f);
    ASSERT_EQ(t3.receptivity, false);
    ASSERT_STREQ(t3.key.c_str(), "T42");
    ASSERT_STREQ(t3.caption.c_str(), "Hello");
    ASSERT_EQ(t3.arcsIn.size(), 0u);
    ASSERT_EQ(t3.arcsOut.size(), 0u);
    ASSERT_EQ(t3.canFire(), false);
    ASSERT_EQ(t3.isInput(), false);
    ASSERT_EQ(t3.isOutput(), false);
    ASSERT_EQ(t3.isState(), false);

    // Check the operator!=() and operator==()
    ASSERT_EQ(t1 == t3, true);
    ASSERT_EQ(t1 != t3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestArcCreation)
{
    Transition t1(42u, "", 3.5f, 4.0f, 45u, true);
    Place p1(43u, "", 4.6f, 5.1f, 13u);

    // Check the default constructor: Transition --> Place
    Arc a1(t1, p1, 10.0f);
    ASSERT_EQ(a1.duration, 10.0f);
    ASSERT_EQ(a1.from.id, 42u);
    ASSERT_EQ(a1.from.type, Node::Transition);
    ASSERT_EQ(a1.from.x, 3.5f);
    ASSERT_EQ(a1.from.y, 4.0f);
    ASSERT_STREQ(a1.from.key.c_str(), "T42");
    ASSERT_STREQ(a1.from.caption.c_str(), "T42");
    ASSERT_EQ(a1.from.arcsIn.size(), 0u);
    ASSERT_EQ(a1.from.arcsOut.size(), 0u);
    ASSERT_EQ(a1.to.id, 43u);
    ASSERT_EQ(a1.to.type, Node::Place);
    ASSERT_EQ(a1.to.x, 4.6f);
    ASSERT_EQ(a1.to.y, 5.1f);
    ASSERT_STREQ(a1.to.key.c_str(), "P43");
    ASSERT_STREQ(a1.to.caption.c_str(), "P43");
    ASSERT_EQ(a1.to.arcsIn.size(), 0u);
    ASSERT_EQ(a1.to.arcsOut.size(), 0u);
    ASSERT_EQ(reinterpret_cast<Transition&>(a1.from).angle, 45);
    ASSERT_EQ(reinterpret_cast<Place&>(a1.to).tokens, 13u);
    ASSERT_EQ(a1.tokensOut(), 13u);

    // Check the default constructor: Place --> Transition
    Arc a2(p1, t1, 15.0f);
    ASSERT_EQ(isnan(a2.duration), true);
    ASSERT_EQ(a2.to.id, 42u);
    ASSERT_EQ(a2.to.type, Node::Transition);
    ASSERT_EQ(a2.to.x, 3.5f);
    ASSERT_EQ(a2.to.y, 4.0f);
    ASSERT_STREQ(a2.to.key.c_str(), "T42");
    ASSERT_STREQ(a2.to.caption.c_str(), "T42");
    ASSERT_EQ(a2.to.arcsIn.size(), 0u);
    ASSERT_EQ(a2.to.arcsOut.size(), 0u);
    ASSERT_EQ(a2.from.id, 43u);
    ASSERT_EQ(a2.from.type, Node::Place);
    ASSERT_EQ(a2.from.x, 4.6f);
    ASSERT_EQ(a2.from.y, 5.1f);
    ASSERT_STREQ(a2.from.key.c_str(), "P43");
    ASSERT_STREQ(a2.from.caption.c_str(), "P43");
    ASSERT_EQ(a2.from.arcsIn.size(), 0u);
    ASSERT_EQ(a2.from.arcsOut.size(), 0u);
    ASSERT_EQ(reinterpret_cast<Transition&>(a2.to).angle, 45);
    ASSERT_EQ(reinterpret_cast<Place&>(a2.from).tokens, 13u);
    ASSERT_EQ(a2.tokensIn(), 13u);

    // Check the operator!=() and operator==()
    ASSERT_EQ(a1 == a2, false);
    ASSERT_EQ(a1 != a2, true);

    // Check the copy constructor
    Arc a3(a1);
    ASSERT_EQ(a3.duration, 10.0f);
    ASSERT_EQ(a3.from.id, 42u);
    ASSERT_EQ(a3.from.type, Node::Transition);
    ASSERT_EQ(a3.from.x, 3.5f);
    ASSERT_EQ(a3.from.y, 4.0f);
    ASSERT_STREQ(a3.from.key.c_str(), "T42");
    ASSERT_STREQ(a3.from.caption.c_str(), "T42");
    ASSERT_EQ(a3.from.arcsIn.size(), 0u);
    ASSERT_EQ(a3.from.arcsOut.size(), 0u);
    ASSERT_EQ(a3.to.id, 43u);
    ASSERT_EQ(a3.to.type, Node::Place);
    ASSERT_EQ(a3.to.x, 4.6f);
    ASSERT_EQ(a3.to.y, 5.1f);
    ASSERT_STREQ(a3.to.key.c_str(), "P43");
    ASSERT_STREQ(a3.to.caption.c_str(), "P43");
    ASSERT_EQ(a3.to.arcsIn.size(), 0u);
    ASSERT_EQ(a3.to.arcsOut.size(), 0u);
    ASSERT_EQ(reinterpret_cast<Transition&>(a3.from).angle, 45);
    ASSERT_EQ(reinterpret_cast<Place&>(a3.to).tokens, 13u);
    ASSERT_EQ(a3.tokensOut(), 13u);

    // Check the copy operator
    Arc a4(p1, t1, 15.0f);
    a4 = a1;
    ASSERT_EQ(a4.duration, 10.0f);
    ASSERT_EQ(a4.from.id, 42u);
    ASSERT_EQ(a4.from.type, Node::Transition);
    ASSERT_EQ(a4.from.x, 3.5f);
    ASSERT_EQ(a4.from.y, 4.0f);
    ASSERT_STREQ(a4.from.key.c_str(), "T42");
    ASSERT_STREQ(a4.from.caption.c_str(), "T42");
    ASSERT_EQ(a4.from.arcsIn.size(), 0u);
    ASSERT_EQ(a4.from.arcsOut.size(), 0u);
    ASSERT_EQ(a4.to.id, 43u);
    ASSERT_EQ(a4.to.type, Node::Place);
    ASSERT_EQ(a4.to.x, 4.6f);
    ASSERT_EQ(a4.to.y, 5.1f);
    ASSERT_STREQ(a4.to.key.c_str(), "P43");
    ASSERT_STREQ(a4.to.caption.c_str(), "P43");
    ASSERT_EQ(a4.to.arcsIn.size(), 0u);
    ASSERT_EQ(a4.to.arcsOut.size(), 0u);
    ASSERT_EQ(reinterpret_cast<Transition&>(a4.from).angle, 45);
    ASSERT_EQ(reinterpret_cast<Place&>(a4.to).tokens, 13u);
    ASSERT_EQ(a4.tokensOut(), 13u);

    // Check the operator!=() and operator==()
    ASSERT_EQ(a1 == a4, true);
    ASSERT_EQ(a1 != a4, false);
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
    ASSERT_STREQ(PetriNet::to_str(PetriNet::Type::GRAFCET).c_str(), "GRAFCET");
    ASSERT_STREQ(PetriNet::to_str(PetriNet::Type::Petri).c_str(), "Petri net");
    ASSERT_STREQ(PetriNet::to_str(PetriNet::Type::TimedPetri).c_str(), "Timed Petri net");
    ASSERT_STREQ(PetriNet::to_str(PetriNet::Type::TimedEventGraph).c_str(), "Timed event graph");
    ASSERT_STREQ(PetriNet::to_str(PetriNet::Type(42)).c_str(), "Undefined type of net");
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, PetriNetConstructor)
{
    PetriNet timed_net(PetriNet::Type::TimedPetri);
    ASSERT_EQ(timed_net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(timed_net.m_type, PetriNet::Type::TimedPetri);

    PetriNet net(PetriNet::Type::Petri);
    ASSERT_EQ(net.type(), PetriNet::Type::Petri);
    ASSERT_EQ(net.m_type, PetriNet::Type::Petri);

    PetriNet grafcet(PetriNet::Type::GRAFCET);
    ASSERT_EQ(grafcet.type(), PetriNet::Type::GRAFCET);
    ASSERT_EQ(grafcet.m_type, PetriNet::Type::GRAFCET);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, PetriNetDummy)
{
    std::vector<Arc*> erroneous_arcs;

    // Check the default constructor: dummy net
    PetriNet net(PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.m_type, PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(&net.places(), &net.m_places);
    ASSERT_EQ(net.places().size(), 0u);

    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(&net.transitions(), &net.m_transitions);
    ASSERT_EQ(net.transitions().size(), 0u);

    ASSERT_EQ(net.m_shuffled_transitions.size(), 0u);

    ASSERT_EQ(net.m_arcs.size(), 0u);
    ASSERT_EQ(&net.arcs(), &net.m_arcs);
    ASSERT_EQ(net.arcs().size(), 0u);

    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);

    ASSERT_EQ(net.modified, false);

    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.isEventGraph(erroneous_arcs), false);
    ASSERT_EQ(erroneous_arcs.empty(), true);

    ASSERT_EQ(net.findNode("P0"), nullptr);
    ASSERT_EQ(net.findNode("T0"), nullptr);
    ASSERT_EQ(net.findNode("pouet"), nullptr);
    ASSERT_EQ(net.findNode(""), nullptr);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestAddRemoveOperations)
{
    std::vector<Arc*> erroneous_arcs;

    // Check the default constructor: dummy net
    PetriNet net(PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);

    // Add Place 0: net = P0
    Place& p0 = net.addPlace(3.14f, 2.16f, 10u);
    ASSERT_EQ(net.m_next_place_id, 1u);
    ASSERT_EQ(p0.id, 0u);
    ASSERT_STREQ(p0.key.c_str(), "P0");
    ASSERT_STREQ(p0.caption.c_str(), "P0");
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.isEventGraph(erroneous_arcs), false);
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
    ASSERT_EQ(net.isEventGraph(erroneous_arcs), false);
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
    ASSERT_EQ(net.addArc(p0, *t0), true);
    ASSERT_EQ(net.addArc(*t0, p1), true);
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
    ASSERT_EQ(net.isEventGraph(erroneous_arcs), false);
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_EQ(net.findNode("T0"), t0);
    ASSERT_EQ(net.m_transitions.size(), 1u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");

    // Add arcs back: net = P0--T0--P1
    ASSERT_EQ(net.addArc(p0, *t0), true);
    ASSERT_EQ(net.addArc(*t0, p1), true);
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
    ASSERT_STREQ(net.m_arcs[0].from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_arcs[0].to.key.c_str(), "P1");
    ASSERT_STREQ(net.m_arcs[0].from.caption.c_str(), "T0");
    ASSERT_STREQ(net.m_arcs[0].to.caption.c_str(), "P1");

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
    ASSERT_EQ(net.removeArc(p0, *t0), false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestCaptionAfterRemove)
{
    // Add P0 and P1
    PetriNet net(PetriNet::Type::TimedPetri);
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
    ASSERT_EQ(net.m_places[0].x, 1.0f); // Attributes from P1
    ASSERT_EQ(net.m_places[0].y, 1.5f);
    ASSERT_EQ(net.m_places[0].tokens, 42u);

    // Add P2. Check we have P0 P2
    net.addPlace(2u, "", 2.0f, 2.5f, 24u);
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "World");
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P2");

    // Remove P0 check P2 is now P0 but without its caption
    net.removeNode(net.m_places[0]);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
    ASSERT_EQ(net.m_places[0].x, 2.0f); // Attributes from P2
    ASSERT_EQ(net.m_places[0].y, 2.5f);
    ASSERT_EQ(net.m_places[0].tokens, 24u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestIncrementId)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    net.addPlace(42u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);

    net.addPlace(42u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);

    net.addPlace(25u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);

    net.addPlace(3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 44u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestDoubleAdd)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    Place& p1 = net.addPlace(42u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 0u);

    /*Place& p2 =*/ net.addPlace(42u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_STREQ(net.m_places[0].key.c_str(), "P42");
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P42");

    Transition& t1 = net.addTransition(43u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 44u);

    /*Transition& t2 =*/ net.addTransition(43u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 44u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T43");
    ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T43");

    ASSERT_EQ(net.addArc(t1, p1), true);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 44u);
    ASSERT_EQ(net.m_arcs.size(), 1u);

    ASSERT_EQ(net.addArc(t1, p1), false);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 44u);
    ASSERT_EQ(net.m_arcs.size(), 1u);

    ASSERT_EQ(net.addArc(p1, t1), true);
    ASSERT_EQ(net.m_next_place_id, 43u);
    ASSERT_EQ(net.m_next_transition_id, 44u);
    ASSERT_EQ(net.m_arcs.size(), 2u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestInvalidAddArc)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    Transition t1(42u, "", 3.5f, 4.0f, 45u, true);
    Transition t2(43u, "", 3.5f, 4.0f, 45u, false);
    Place p1(44u, "", 4.6f, 5.1f, 13u);
    Place p2(45u, "", 4.6f, 5.1f, 13u);

    // Bad arc: Transition --> transition
    ASSERT_EQ(net.addArc(t1, t2), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.addArc(t1, t1), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // Bad arc: Place --> Place
    ASSERT_EQ(net.addArc(p1, p2), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.addArc(p1, p1), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    // Bad arc: double insertion
    ASSERT_EQ(net.addArc(t1, p2), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.addArc(p2, t1), false);
    ASSERT_EQ(net.isEmpty(), true);
    ASSERT_EQ(net.m_next_place_id, 0u);
    ASSERT_EQ(net.m_next_transition_id, 0u);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);

    Transition& t3 = net.addTransition(43u, "", 3.5f, 4.0f, 45u);
    ASSERT_EQ(net.addArc(t3, p2), false);
    ASSERT_EQ(net.m_arcs.size(), 0u);

}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadedNetTimedPetri)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/Howard2.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
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
    ASSERT_EQ(net.m_places[0].type, Node::Place);
    ASSERT_EQ(net.m_places[0].tokens, 2u);
    ASSERT_EQ(net.m_places[0].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[0].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[0].arcsIn[0]->from.key.c_str(), "T2");
    ASSERT_STREQ(net.m_places[0].arcsIn[0]->to.key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].arcsOut[0]->from.key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].arcsOut[0]->to.key.c_str(), "T0");

    ASSERT_EQ(net.m_places[1].id, 1u);
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
    ASSERT_EQ(net.m_places[1].type, Node::Place);
    ASSERT_EQ(net.m_places[1].tokens, 0u);
    ASSERT_EQ(net.m_places[1].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[1].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[1].arcsIn[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_places[1].arcsIn[0]->to.key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].arcsOut[0]->from.key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].arcsOut[0]->to.key.c_str(), "T1");

    ASSERT_EQ(net.m_places[2].id, 2u);
    ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].caption.c_str(), "P2");
    ASSERT_EQ(net.m_places[2].type, Node::Place);
    ASSERT_EQ(net.m_places[2].tokens, 0u);
    ASSERT_EQ(net.m_places[2].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[2].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[2].arcsIn[0]->from.key.c_str(), "T1");
    ASSERT_STREQ(net.m_places[2].arcsIn[0]->to.key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].arcsOut[0]->from.key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].arcsOut[0]->to.key.c_str(), "T2");

    ASSERT_EQ(net.m_places[3].id, 3u);
    ASSERT_STREQ(net.m_places[3].key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].caption.c_str(), "P3");
    ASSERT_EQ(net.m_places[3].type, Node::Place);
    ASSERT_EQ(net.m_places[3].tokens, 0u);
    ASSERT_EQ(net.m_places[3].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[3].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[3].arcsIn[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_places[3].arcsIn[0]->to.key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].arcsOut[0]->from.key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].arcsOut[0]->to.key.c_str(), "T3");

    ASSERT_EQ(net.m_places[4].id, 4u);
    ASSERT_STREQ(net.m_places[4].key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].caption.c_str(), "P4");
    ASSERT_EQ(net.m_places[4].type, Node::Place);
    ASSERT_EQ(net.m_places[4].tokens, 0u);
    ASSERT_EQ(net.m_places[4].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[4].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[4].arcsIn[0]->from.key.c_str(), "T3");
    ASSERT_STREQ(net.m_places[4].arcsIn[0]->to.key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].arcsOut[0]->from.key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].arcsOut[0]->to.key.c_str(), "T2");

    // Check transitions
    ASSERT_EQ(net.m_transitions[0].id, 0u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");
    ASSERT_EQ(net.m_transitions[0].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[0].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[0].arcsOut.size(), 2u);
    ASSERT_STREQ(net.m_transitions[0].arcsIn[0]->from.key.c_str(), "P0");
    ASSERT_STREQ(net.m_transitions[0].arcsIn[0]->to.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[0]->to.key.c_str(), "P1");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[1]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[1]->to.key.c_str(), "P3");

    ASSERT_EQ(net.m_transitions[1].id, 1u);
    ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "T1");
    ASSERT_EQ(net.m_transitions[1].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[1].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[1].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[1].arcsIn[0]->from.key.c_str(), "P1");
    ASSERT_STREQ(net.m_transitions[1].arcsIn[0]->to.key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].arcsOut[0]->from.key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].arcsOut[0]->to.key.c_str(), "P2");

    ASSERT_EQ(net.m_transitions[2].id, 2u);
    ASSERT_STREQ(net.m_transitions[2].key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].caption.c_str(), "T2");
    ASSERT_EQ(net.m_transitions[2].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[2].arcsIn.size(), 2u);
    ASSERT_EQ(net.m_transitions[2].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[2].arcsIn[0]->from.key.c_str(), "P2");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[0]->to.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[1]->from.key.c_str(), "P4");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[1]->to.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsOut[0]->from.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsOut[0]->to.key.c_str(), "P0");

    ASSERT_EQ(net.m_transitions[3].id, 3u);
    ASSERT_STREQ(net.m_transitions[3].key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].caption.c_str(), "T3");
    ASSERT_EQ(net.m_transitions[3].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[3].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[3].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[3].arcsIn[0]->from.key.c_str(), "P3");
    ASSERT_STREQ(net.m_transitions[3].arcsIn[0]->to.key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].arcsOut[0]->from.key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].arcsOut[0]->to.key.c_str(), "P4");

    // Check arcs
    ASSERT_EQ(net.m_arcs[0].from.id, 0u);
    ASSERT_EQ(net.m_arcs[0].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[0].from.key.c_str(), "P0");
    ASSERT_EQ(net.m_arcs[0].to.id, 0u);
    ASSERT_EQ(net.m_arcs[0].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[0].to.key.c_str(), "T0");
    ASSERT_EQ(isnan(net.m_arcs[0].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[1].from.id, 0u);
    ASSERT_EQ(net.m_arcs[1].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[1].from.key.c_str(), "T0");
    ASSERT_EQ(net.m_arcs[1].to.id, 1u);
    ASSERT_EQ(net.m_arcs[1].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[1].to.key.c_str(), "P1");
    ASSERT_EQ(net.m_arcs[1].duration, 5u);

    ASSERT_EQ(net.m_arcs[2].from.id, 1u);
    ASSERT_EQ(net.m_arcs[2].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[2].from.key.c_str(), "P1");
    ASSERT_EQ(net.m_arcs[2].to.id, 1u);
    ASSERT_EQ(net.m_arcs[2].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[2].to.key.c_str(), "T1");
    ASSERT_EQ(isnan(net.m_arcs[2].duration), true);

    ASSERT_EQ(net.m_arcs[3].from.id, 1u);
    ASSERT_EQ(net.m_arcs[3].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[3].from.key.c_str(), "T1");
    ASSERT_EQ(net.m_arcs[3].to.id, 2u);
    ASSERT_EQ(net.m_arcs[3].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[3].to.key.c_str(), "P2");
    ASSERT_EQ(net.m_arcs[3].duration, 3u);

    ASSERT_EQ(net.m_arcs[4].from.id, 2u);
    ASSERT_EQ(net.m_arcs[4].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[4].from.key.c_str(), "P2");
    ASSERT_EQ(net.m_arcs[4].to.id, 2u);
    ASSERT_EQ(net.m_arcs[4].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[4].to.key.c_str(), "T2");
    ASSERT_EQ(isnan(net.m_arcs[4].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[5].from.id, 2u);
    ASSERT_EQ(net.m_arcs[5].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[5].from.key.c_str(), "T2");
    ASSERT_EQ(net.m_arcs[5].to.id, 0u);
    ASSERT_EQ(net.m_arcs[5].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[5].to.key.c_str(), "P0");
    ASSERT_EQ(net.m_arcs[5].duration, 5u);

    ASSERT_EQ(net.m_arcs[6].from.id, 0u);
    ASSERT_EQ(net.m_arcs[6].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[6].from.key.c_str(), "T0");
    ASSERT_EQ(net.m_arcs[6].to.id, 3u);
    ASSERT_EQ(net.m_arcs[6].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[6].to.key.c_str(), "P3");
    ASSERT_EQ(net.m_arcs[6].duration, 1u);

    ASSERT_EQ(net.m_arcs[7].from.id, 3u);
    ASSERT_EQ(net.m_arcs[7].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[7].from.key.c_str(), "P3");
    ASSERT_EQ(net.m_arcs[7].to.id, 3u);
    ASSERT_EQ(net.m_arcs[7].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[7].to.key.c_str(), "T3");
    ASSERT_EQ(isnan(net.m_arcs[7].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[8].from.id, 3u);
    ASSERT_EQ(net.m_arcs[8].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[8].from.key.c_str(), "T3");
    ASSERT_EQ(net.m_arcs[8].to.id, 4u);
    ASSERT_EQ(net.m_arcs[8].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[8].to.key.c_str(), "P4");
    ASSERT_EQ(net.m_arcs[8].duration, 1u);

    ASSERT_EQ(net.m_arcs[9].from.id, 4u);
    ASSERT_EQ(net.m_arcs[9].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[9].from.key.c_str(), "P4");
    ASSERT_EQ(net.m_arcs[9].to.id, 2u);
    ASSERT_EQ(net.m_arcs[9].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[9].to.key.c_str(), "T2");
    ASSERT_EQ(isnan(net.m_arcs[9].duration), true); // FIXME forcer json a NAN ?

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
    ASSERT_STREQ(arc->from.key.c_str(), "T0");
    ASSERT_STREQ(arc->to.key.c_str(), "P1");

    arc = net.findArc(*net.findNode("T0"), *net.findNode("P3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T0");
    ASSERT_STREQ(arc->to.key.c_str(), "P3");

    arc = net.findArc(*net.findNode("T1"), *net.findNode("P2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T1");
    ASSERT_STREQ(arc->to.key.c_str(), "P2");

    arc = net.findArc(*net.findNode("T2"), *net.findNode("P0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T2");
    ASSERT_STREQ(arc->to.key.c_str(), "P0");

    arc = net.findArc(*net.findNode("T3"), *net.findNode("P4"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T3");
    ASSERT_STREQ(arc->to.key.c_str(), "P4");

    arc = net.findArc(*net.findNode("P0"), *net.findNode("T0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P0");
    ASSERT_STREQ(arc->to.key.c_str(), "T0");

    arc = net.findArc(*net.findNode("P1"), *net.findNode("T1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P1");
    ASSERT_STREQ(arc->to.key.c_str(), "T1");

    arc = net.findArc(*net.findNode("P2"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P2");
    ASSERT_STREQ(arc->to.key.c_str(), "T2");

    arc = net.findArc(*net.findNode("P3"), *net.findNode("T3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P3");
    ASSERT_STREQ(arc->to.key.c_str(), "T3");

    arc = net.findArc(*net.findNode("P4"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P4");
    ASSERT_STREQ(arc->to.key.c_str(), "T2");

    // At least one token
    ASSERT_EQ(net.m_transitions[0].isEnabled(), true);
    ASSERT_EQ(net.m_transitions[1].isEnabled(), false);
    ASSERT_EQ(net.m_transitions[2].isEnabled(), false);
    ASSERT_EQ(net.m_transitions[3].isEnabled(), false);

    // Receptivity
    ASSERT_EQ(net.m_transitions[0].isValidated(), true);
    ASSERT_EQ(net.m_transitions[1].isValidated(), true);
    ASSERT_EQ(net.m_transitions[2].isValidated(), true);
    ASSERT_EQ(net.m_transitions[3].isValidated(), true);

    // Can fire ? (Version 1)
    ASSERT_EQ(net.m_transitions[0].canFire(), true);
    ASSERT_EQ(net.m_transitions[1].canFire(), false);
    ASSERT_EQ(net.m_transitions[2].canFire(), false);
    ASSERT_EQ(net.m_transitions[3].canFire(), false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadedNetGraphEvent)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/EventGraph2.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedEventGraph);
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
    ASSERT_EQ(net.m_places[0].type, Node::Place);
    ASSERT_EQ(net.m_places[0].tokens, 2u);
    ASSERT_EQ(net.m_places[0].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[0].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[0].arcsIn[0]->from.key.c_str(), "T2");
    ASSERT_STREQ(net.m_places[0].arcsIn[0]->to.key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].arcsOut[0]->from.key.c_str(), "P0");
    ASSERT_STREQ(net.m_places[0].arcsOut[0]->to.key.c_str(), "T0");

    ASSERT_EQ(net.m_places[1].id, 1u);
    ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
    ASSERT_EQ(net.m_places[1].type, Node::Place);
    ASSERT_EQ(net.m_places[1].tokens, 0u);
    ASSERT_EQ(net.m_places[1].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[1].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[1].arcsIn[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_places[1].arcsIn[0]->to.key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].arcsOut[0]->from.key.c_str(), "P1");
    ASSERT_STREQ(net.m_places[1].arcsOut[0]->to.key.c_str(), "T1");

    ASSERT_EQ(net.m_places[2].id, 2u);
    ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].caption.c_str(), "P2");
    ASSERT_EQ(net.m_places[2].type, Node::Place);
    ASSERT_EQ(net.m_places[2].tokens, 0u);
    ASSERT_EQ(net.m_places[2].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[2].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[2].arcsIn[0]->from.key.c_str(), "T1");
    ASSERT_STREQ(net.m_places[2].arcsIn[0]->to.key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].arcsOut[0]->from.key.c_str(), "P2");
    ASSERT_STREQ(net.m_places[2].arcsOut[0]->to.key.c_str(), "T2");

    ASSERT_EQ(net.m_places[3].id, 3u);
    ASSERT_STREQ(net.m_places[3].key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].caption.c_str(), "P3");
    ASSERT_EQ(net.m_places[3].type, Node::Place);
    ASSERT_EQ(net.m_places[3].tokens, 0u);
    ASSERT_EQ(net.m_places[3].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[3].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[3].arcsIn[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_places[3].arcsIn[0]->to.key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].arcsOut[0]->from.key.c_str(), "P3");
    ASSERT_STREQ(net.m_places[3].arcsOut[0]->to.key.c_str(), "T3");

    ASSERT_EQ(net.m_places[4].id, 4u);
    ASSERT_STREQ(net.m_places[4].key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].caption.c_str(), "P4");
    ASSERT_EQ(net.m_places[4].type, Node::Place);
    ASSERT_EQ(net.m_places[4].tokens, 0u);
    ASSERT_EQ(net.m_places[4].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_places[4].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_places[4].arcsIn[0]->from.key.c_str(), "T3");
    ASSERT_STREQ(net.m_places[4].arcsIn[0]->to.key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].arcsOut[0]->from.key.c_str(), "P4");
    ASSERT_STREQ(net.m_places[4].arcsOut[0]->to.key.c_str(), "T2");

    // Check transitions
    ASSERT_EQ(net.m_transitions[0].id, 0u);
    ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");
    ASSERT_EQ(net.m_transitions[0].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[0].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[0].arcsOut.size(), 2u);
    ASSERT_STREQ(net.m_transitions[0].arcsIn[0]->from.key.c_str(), "P0");
    ASSERT_STREQ(net.m_transitions[0].arcsIn[0]->to.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[0]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[0]->to.key.c_str(), "P1");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[1]->from.key.c_str(), "T0");
    ASSERT_STREQ(net.m_transitions[0].arcsOut[1]->to.key.c_str(), "P3");

    ASSERT_EQ(net.m_transitions[1].id, 1u);
    ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "T1");
    ASSERT_EQ(net.m_transitions[1].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[1].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[1].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[1].arcsIn[0]->from.key.c_str(), "P1");
    ASSERT_STREQ(net.m_transitions[1].arcsIn[0]->to.key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].arcsOut[0]->from.key.c_str(), "T1");
    ASSERT_STREQ(net.m_transitions[1].arcsOut[0]->to.key.c_str(), "P2");

    ASSERT_EQ(net.m_transitions[2].id, 2u);
    ASSERT_STREQ(net.m_transitions[2].key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].caption.c_str(), "T2");
    ASSERT_EQ(net.m_transitions[2].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[2].arcsIn.size(), 2u);
    ASSERT_EQ(net.m_transitions[2].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[2].arcsIn[0]->from.key.c_str(), "P2");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[0]->to.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[1]->from.key.c_str(), "P4");
    ASSERT_STREQ(net.m_transitions[2].arcsIn[1]->to.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsOut[0]->from.key.c_str(), "T2");
    ASSERT_STREQ(net.m_transitions[2].arcsOut[0]->to.key.c_str(), "P0");

    ASSERT_EQ(net.m_transitions[3].id, 3u);
    ASSERT_STREQ(net.m_transitions[3].key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].caption.c_str(), "T3");
    ASSERT_EQ(net.m_transitions[3].type, Node::Transition);
    ASSERT_EQ(net.m_transitions[3].arcsIn.size(), 1u);
    ASSERT_EQ(net.m_transitions[3].arcsOut.size(), 1u);
    ASSERT_STREQ(net.m_transitions[3].arcsIn[0]->from.key.c_str(), "P3");
    ASSERT_STREQ(net.m_transitions[3].arcsIn[0]->to.key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].arcsOut[0]->from.key.c_str(), "T3");
    ASSERT_STREQ(net.m_transitions[3].arcsOut[0]->to.key.c_str(), "P4");

    // Check arcs
    ASSERT_EQ(net.m_arcs[0].from.id, 0u);
    ASSERT_EQ(net.m_arcs[0].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[0].from.key.c_str(), "P0");
    ASSERT_EQ(net.m_arcs[0].to.id, 0u);
    ASSERT_EQ(net.m_arcs[0].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[0].to.key.c_str(), "T0");
    ASSERT_EQ(isnan(net.m_arcs[0].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[1].from.id, 0u);
    ASSERT_EQ(net.m_arcs[1].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[1].from.key.c_str(), "T0");
    ASSERT_EQ(net.m_arcs[1].to.id, 1u);
    ASSERT_EQ(net.m_arcs[1].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[1].to.key.c_str(), "P1");
    ASSERT_EQ(net.m_arcs[1].duration, 5u);

    ASSERT_EQ(net.m_arcs[2].from.id, 1u);
    ASSERT_EQ(net.m_arcs[2].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[2].from.key.c_str(), "P1");
    ASSERT_EQ(net.m_arcs[2].to.id, 1u);
    ASSERT_EQ(net.m_arcs[2].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[2].to.key.c_str(), "T1");
    ASSERT_EQ(isnan(net.m_arcs[2].duration), true);

    ASSERT_EQ(net.m_arcs[3].from.id, 1u);
    ASSERT_EQ(net.m_arcs[3].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[3].from.key.c_str(), "T1");
    ASSERT_EQ(net.m_arcs[3].to.id, 2u);
    ASSERT_EQ(net.m_arcs[3].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[3].to.key.c_str(), "P2");
    ASSERT_EQ(net.m_arcs[3].duration, 3u);

    ASSERT_EQ(net.m_arcs[4].from.id, 2u);
    ASSERT_EQ(net.m_arcs[4].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[4].from.key.c_str(), "P2");
    ASSERT_EQ(net.m_arcs[4].to.id, 2u);
    ASSERT_EQ(net.m_arcs[4].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[4].to.key.c_str(), "T2");
    ASSERT_EQ(isnan(net.m_arcs[4].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[5].from.id, 2u);
    ASSERT_EQ(net.m_arcs[5].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[5].from.key.c_str(), "T2");
    ASSERT_EQ(net.m_arcs[5].to.id, 0u);
    ASSERT_EQ(net.m_arcs[5].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[5].to.key.c_str(), "P0");
    ASSERT_EQ(net.m_arcs[5].duration, 5u);

    ASSERT_EQ(net.m_arcs[6].from.id, 0u);
    ASSERT_EQ(net.m_arcs[6].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[6].from.key.c_str(), "T0");
    ASSERT_EQ(net.m_arcs[6].to.id, 3u);
    ASSERT_EQ(net.m_arcs[6].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[6].to.key.c_str(), "P3");
    ASSERT_EQ(net.m_arcs[6].duration, 1u);

    ASSERT_EQ(net.m_arcs[7].from.id, 3u);
    ASSERT_EQ(net.m_arcs[7].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[7].from.key.c_str(), "P3");
    ASSERT_EQ(net.m_arcs[7].to.id, 3u);
    ASSERT_EQ(net.m_arcs[7].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[7].to.key.c_str(), "T3");
    ASSERT_EQ(isnan(net.m_arcs[7].duration), true); // FIXME forcer json a NAN ?

    ASSERT_EQ(net.m_arcs[8].from.id, 3u);
    ASSERT_EQ(net.m_arcs[8].from.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[8].from.key.c_str(), "T3");
    ASSERT_EQ(net.m_arcs[8].to.id, 4u);
    ASSERT_EQ(net.m_arcs[8].to.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[8].to.key.c_str(), "P4");
    ASSERT_EQ(net.m_arcs[8].duration, 1u);

    ASSERT_EQ(net.m_arcs[9].from.id, 4u);
    ASSERT_EQ(net.m_arcs[9].from.type, Node::Place);
    ASSERT_STREQ(net.m_arcs[9].from.key.c_str(), "P4");
    ASSERT_EQ(net.m_arcs[9].to.id, 2u);
    ASSERT_EQ(net.m_arcs[9].to.type, Node::Transition);
    ASSERT_STREQ(net.m_arcs[9].to.key.c_str(), "T2");
    ASSERT_EQ(isnan(net.m_arcs[9].duration), true); // FIXME forcer json a NAN ?

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
    ASSERT_STREQ(arc->from.key.c_str(), "T0");
    ASSERT_STREQ(arc->to.key.c_str(), "P1");

    arc = net.findArc(*net.findNode("T0"), *net.findNode("P3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T0");
    ASSERT_STREQ(arc->to.key.c_str(), "P3");

    arc = net.findArc(*net.findNode("T1"), *net.findNode("P2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T1");
    ASSERT_STREQ(arc->to.key.c_str(), "P2");

    arc = net.findArc(*net.findNode("T2"), *net.findNode("P0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T2");
    ASSERT_STREQ(arc->to.key.c_str(), "P0");

    arc = net.findArc(*net.findNode("T3"), *net.findNode("P4"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "T3");
    ASSERT_STREQ(arc->to.key.c_str(), "P4");

    arc = net.findArc(*net.findNode("P0"), *net.findNode("T0"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P0");
    ASSERT_STREQ(arc->to.key.c_str(), "T0");

    arc = net.findArc(*net.findNode("P1"), *net.findNode("T1"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P1");
    ASSERT_STREQ(arc->to.key.c_str(), "T1");

    arc = net.findArc(*net.findNode("P2"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P2");
    ASSERT_STREQ(arc->to.key.c_str(), "T2");

    arc = net.findArc(*net.findNode("P3"), *net.findNode("T3"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P3");
    ASSERT_STREQ(arc->to.key.c_str(), "T3");

    arc = net.findArc(*net.findNode("P4"), *net.findNode("T2"));
    ASSERT_NE(arc, nullptr);
    ASSERT_STREQ(arc->from.key.c_str(), "P4");
    ASSERT_STREQ(arc->to.key.c_str(), "T2");

    // At least one token
    ASSERT_EQ(net.m_transitions[0].isEnabled(), true);
    ASSERT_EQ(net.m_transitions[1].isEnabled(), false);
    ASSERT_EQ(net.m_transitions[2].isEnabled(), false);
    ASSERT_EQ(net.m_transitions[3].isEnabled(), false);

    // Receptivity
    ASSERT_EQ(net.m_transitions[0].isValidated(), true);
    ASSERT_EQ(net.m_transitions[1].isValidated(), true);
    ASSERT_EQ(net.m_transitions[2].isValidated(), true);
    ASSERT_EQ(net.m_transitions[3].isValidated(), true);

    // Can fire ? (Version 1)
    ASSERT_EQ(net.m_transitions[0].canFire(), true);
    ASSERT_EQ(net.m_transitions[1].canFire(), false);
    ASSERT_EQ(net.m_transitions[2].canFire(), false);
    ASSERT_EQ(net.m_transitions[3].canFire(), false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestRemoveNode)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/Howard2.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 4u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 4u);
    ASSERT_EQ(net.m_arcs.size(), 10u);

    // Delete Transition 0
    net.removeNode(*net.findNode("T0"));
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 3u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 3u);
    ASSERT_EQ(net.m_arcs.size(), 7u);

    // Check Places
    ASSERT_NE(net.findPlace(0u), nullptr);
    ASSERT_NE(net.findPlace(1u), nullptr);
    ASSERT_NE(net.findPlace(2u), nullptr);
    ASSERT_NE(net.findPlace(3u), nullptr);
    ASSERT_NE(net.findPlace(4u), nullptr);
    ASSERT_EQ(net.findPlace(5u), nullptr);

    // Check Transitions
    ASSERT_NE(net.findTransition(0u), nullptr);
    ASSERT_NE(net.findTransition(1u), nullptr);
    ASSERT_NE(net.findTransition(2u), nullptr);
    ASSERT_EQ(net.findTransition(3u), nullptr);

    // Check arcs
    ASSERT_NE(net.findArc(*net.findTransition(2u), *net.findPlace(0u)), nullptr);
    ASSERT_NE(net.findArc(*net.findTransition(1u), *net.findPlace(2u)), nullptr);
    ASSERT_NE(net.findArc(*net.findTransition(0u), *net.findPlace(4u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(1u), *net.findTransition(1u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(2u), *net.findTransition(2u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(3u), *net.findTransition(0u)), nullptr);
    ASSERT_NE(net.findArc(*net.findPlace(4u), *net.findTransition(2u)), nullptr);

    // In/out arcs Transition
    // T0 (previously T3)
    {
        auto const& arcsIn = net.findTransition(0u)->arcsIn;
        auto const& arcsOut = net.findTransition(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P3");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P4");
        ASSERT_EQ(arcsOut[0]->duration, 1.0f);
    }
    // T1
    {
        auto const& arcsIn = net.findTransition(1u)->arcsIn;
        auto const& arcsOut = net.findTransition(1u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T1");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T1");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P2");
        ASSERT_EQ(arcsOut[0]->duration, 3.0f);
    }
    // T2
    {
        auto const& arcsIn = net.findTransition(2u)->arcsIn;
        auto const& arcsOut = net.findTransition(2u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 2u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P2");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T2");
        ASSERT_STREQ(arcsIn[1]->from.key.c_str(), "P4");
        ASSERT_STREQ(arcsIn[1]->to.key.c_str(), "T2");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T2");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P0");
        ASSERT_EQ(arcsOut[0]->duration, 5.0f);
    }

    // In/out arcs Place
    // P0
    {
        auto const& arcsIn = net.findPlace(0u)->arcsIn;
        auto const& arcsOut = net.findPlace(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T2");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P0");
    }
    // P1
    {
        auto const& arcsIn = net.findPlace(1u)->arcsIn;
        auto const& arcsOut = net.findPlace(1u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T1");
    }
    // P2
    {
        auto const& arcsIn = net.findPlace(2u)->arcsIn;
        auto const& arcsOut = net.findPlace(2u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P2");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T2");
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T1");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P2");
    }
    // P3
    {
        auto const& arcsIn = net.findPlace(3u)->arcsIn;
        auto const& arcsOut = net.findPlace(3u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P3");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T0");
    }
    // P4
    {
        auto const& arcsIn = net.findPlace(4u)->arcsIn;
        auto const& arcsOut = net.findPlace(4u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P4");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T2");
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P4");
    }

    // Delete Transition 2
    net.removeNode(*net.findNode("T2"));
    ASSERT_EQ(net.m_next_place_id, 5u);
    ASSERT_EQ(net.m_next_transition_id, 2u);
    ASSERT_EQ(net.m_places.size(), 5u);
    ASSERT_EQ(net.m_transitions.size(), 2u);
    ASSERT_EQ(net.m_arcs.size(), 4u);

    // T0
    {
        auto const& arcsIn = net.findTransition(0u)->arcsIn;
        auto const& arcsOut = net.findTransition(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P3");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P4");
        ASSERT_EQ(arcsOut[0]->duration, 1.0f);
    }
    // T1
    {
        auto const& arcsIn = net.findTransition(1u)->arcsIn;
        auto const& arcsOut = net.findTransition(1u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T1");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T1");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P2");
        ASSERT_EQ(arcsOut[0]->duration, 3.0f);
    }
    // P0
    {
        auto const& arcsIn = net.findPlace(0u)->arcsIn;
        auto const& arcsOut = net.findPlace(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 0u);
    }
    // P1
    {
        auto const& arcsIn = net.findPlace(1u)->arcsIn;
        auto const& arcsOut = net.findPlace(1u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T1");
    }
    // P2
    {
        auto const& arcsIn = net.findPlace(2u)->arcsIn;
        auto const& arcsOut = net.findPlace(2u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T1");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P2");
    }
    // P3
    {
        auto const& arcsIn = net.findPlace(3u)->arcsIn;
        auto const& arcsOut = net.findPlace(3u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P3");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T0");
    }
    // P4
    {
        auto const& arcsIn = net.findPlace(4u)->arcsIn;
        auto const& arcsOut = net.findPlace(4u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P4");
    }

    // Delete Place 0: Place 4 becomes Place 0
    net.removeNode(*net.findNode("P0"));
    ASSERT_EQ(net.m_next_place_id, 4u);
    ASSERT_EQ(net.m_next_transition_id, 2u);
    ASSERT_EQ(net.m_places.size(), 4u);
    ASSERT_EQ(net.m_transitions.size(), 2u);
    ASSERT_EQ(net.m_arcs.size(), 4u);

    // P0
    {
        auto const& arcsIn = net.findPlace(0u)->arcsIn;
        auto const& arcsOut = net.findPlace(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P0");
    }
    // T0
    {
        auto const& arcsIn = net.findTransition(0u)->arcsIn;
        auto const& arcsOut = net.findTransition(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P3");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P0");
        ASSERT_EQ(arcsOut[0]->duration, 1.0f);
    }

    // Delete Place 0: Place 3 becomes Place 0
    net.removeNode(*net.findNode("P0"));

    // P0
    {
        auto const& arcsIn = net.findPlace(0u)->arcsIn;
        auto const& arcsOut = net.findPlace(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P0");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T0");
    }
    // T0
    {
        auto const& arcsIn = net.findTransition(0u)->arcsIn;
        auto const& arcsOut = net.findTransition(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P0");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T0");
    }

    // Delete Transition 0: Transition 1 becomes Place 0
    net.removeNode(*net.findNode("T0"));

    // P0
    {
        auto const& arcsIn = net.findPlace(0u)->arcsIn;
        auto const& arcsOut = net.findPlace(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 0u);
    }
    // P1
    {
        auto const& arcsIn = net.findPlace(1u)->arcsIn;
        auto const& arcsOut = net.findPlace(1u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 0u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "T0");
    }
    // P2
    {
        auto const& arcsIn = net.findPlace(2u)->arcsIn;
        auto const& arcsOut = net.findPlace(2u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 0u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "P2");
        ASSERT_EQ(arcsIn[0]->duration, 3.0f);
    }
    // T0
    {
        auto const& arcsIn = net.findTransition(0u)->arcsIn;
        auto const& arcsOut = net.findTransition(0u)->arcsOut;
        ASSERT_EQ(arcsIn.size(), 1u);
        ASSERT_EQ(arcsOut.size(), 1u);
        ASSERT_STREQ(arcsIn[0]->from.key.c_str(), "P1");
        ASSERT_STREQ(arcsIn[0]->to.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->from.key.c_str(), "T0");
        ASSERT_STREQ(arcsOut[0]->to.key.c_str(), "P2");
    }

    net.removeNode(*net.findNode("T0"));
    net.removeNode(*net.findNode("P2"));
    net.removeNode(*net.findNode("P1"));
    net.removeNode(*net.findNode("P0"));
    ASSERT_EQ(net.isEmpty(), true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestHowManyTokensCanBurnt)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/EventGraph.json"), true);
    net.type(PetriNet::Type::Petri);

    ASSERT_EQ(net.m_transitions.size(), 4u);
    ASSERT_EQ(net.m_transitions[0].howManyTokensCanBurnt(), 1u); // u
    ASSERT_EQ(net.m_transitions[1].howManyTokensCanBurnt(), 0u); // x1
    ASSERT_EQ(net.m_transitions[2].howManyTokensCanBurnt(), 0u); // x2
    ASSERT_EQ(net.m_transitions[3].howManyTokensCanBurnt(), 0u); // y

    // TODO ASSERT_EQ(net.m_transitions[0].fire());
    //ASSERT_EQ(net.m_transitions[0].howManyTokensCanBurnt(), 1u); // u
    //ASSERT_EQ(net.m_transitions[1].howManyTokensCanBurnt(), 2u); // x1
    //ASSERT_EQ(net.m_transitions[2].howManyTokensCanBurnt(), 2u); // x2
    //ASSERT_EQ(net.m_transitions[3].howManyTokensCanBurnt(), 0u); // y
}
