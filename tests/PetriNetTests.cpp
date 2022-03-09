//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#include "main.hpp"
#define protected public
#define private public
#  include "src/PetriNet.hpp"
#undef protected
#undef private

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestNodeCreation)
{
    // Place from mother class
    Node n1(Node::Place, 2u, 3.5f, 4.0f);
    ASSERT_EQ(n1.id, 2u);
    ASSERT_EQ(n1.type, Node::Place);
    ASSERT_EQ(n1.x, 3.5f);
    ASSERT_EQ(n1.y, 4.0f);
    ASSERT_STREQ(n1.key.c_str(), "P2");
    ASSERT_STREQ(n1.caption.c_str(), "P2");
    ASSERT_EQ(n1.arcsIn.size(), 0u);
    ASSERT_EQ(n1.arcsOut.size(), 0u);

    // Transition from mother class
    Node n2(Node::Transition, 42u, 4.0f, 3.5f);
    ASSERT_EQ(n2.id, 42u);
    ASSERT_EQ(n2.type, Node::Transition);
    ASSERT_EQ(n2.x, 4.0f);
    ASSERT_EQ(n2.y, 3.5f);
    ASSERT_STREQ(n2.key.c_str(), "T42");
    ASSERT_STREQ(n2.caption.c_str(), "T42");
    ASSERT_EQ(n2.arcsIn.size(), 0u);
    ASSERT_EQ(n2.arcsOut.size(), 0u);

    // Check != operator
    Node n3(Node::Place, 42u, 4.0f, 3.5f);
    ASSERT_EQ(n1 != n2, true); // Transition vs Place + different id
    ASSERT_EQ(n3 != n2, true); // Transition vs Place + same id
    ASSERT_EQ(n1 != n2, true); // different id

    // Copy operator FIXME: vraiment besoin ?
    n1 = n2;
    ASSERT_EQ(n1.id, 42u);
    ASSERT_EQ(n1.type, Node::Transition);
    ASSERT_EQ(n1.x, 4.0f);
    ASSERT_EQ(n1.y, 3.5f);
    ASSERT_STREQ(n1.key.c_str(), "T42");
    ASSERT_STREQ(n1.caption.c_str(), "T42");
    ASSERT_EQ(n1.arcsIn.size(), 0u);
    ASSERT_EQ(n1.arcsOut.size(), 0u);

    // Copy constructor
    Node n4(n1);
    ASSERT_EQ(n4.id, 42u);
    ASSERT_EQ(n4.type, Node::Transition);
    ASSERT_EQ(n4.x, 4.0f);
    ASSERT_EQ(n4.y, 3.5f);
    ASSERT_STREQ(n4.key.c_str(), "T42");
    ASSERT_STREQ(n4.caption.c_str(), "T42");
    ASSERT_EQ(n4.arcsIn.size(), 0u);
    ASSERT_EQ(n4.arcsOut.size(), 0u);

    // Check, operator
    n4.x = 5.0f; n4.caption = "foo";
    ASSERT_EQ(n1 == n4, true);
    ASSERT_EQ(n1 == n2, true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestPlaceCreation)
{
    Place p1(42u, 3.5f, 4.0f, 12u);
    ASSERT_EQ(p1.id, 42u);
    ASSERT_EQ(p1.type, Node::Place);
    ASSERT_EQ(p1.tokens, 12u);
    ASSERT_EQ(p1.x, 3.5f);
    ASSERT_EQ(p1.y, 4.0f);
    ASSERT_STREQ(p1.key.c_str(), "P42");
    ASSERT_STREQ(p1.caption.c_str(), "P42");
    ASSERT_EQ(p1.arcsIn.size(), 0u);
    ASSERT_EQ(p1.arcsOut.size(), 0u);

    Place p2(p1);
    ASSERT_EQ(p2.id, 42u);
    ASSERT_EQ(p2.type, Node::Place);
    ASSERT_EQ(p2.tokens, 12u);
    ASSERT_EQ(p2.x, 3.5f);
    ASSERT_EQ(p2.y, 4.0f);
    ASSERT_STREQ(p2.key.c_str(), "P42");
    ASSERT_STREQ(p2.caption.c_str(), "P42");
    ASSERT_EQ(p2.arcsIn.size(), 0u);
    ASSERT_EQ(p2.arcsOut.size(), 0u);
    ASSERT_EQ(p1 == p2, true);
    ASSERT_EQ(p1 != p2, false);

    Place p3(0u, 0.0f, 0.0f, 0u);
    ASSERT_EQ(p1 == p3, false);
    ASSERT_EQ(p1 != p3, true);

    p3 = p1;
    ASSERT_EQ(p3.id, 42u);
    ASSERT_EQ(p3.type, Node::Place);
    ASSERT_EQ(p3.tokens, 12u);
    ASSERT_EQ(p3.x, 3.5f);
    ASSERT_EQ(p3.y, 4.0f);
    ASSERT_STREQ(p3.key.c_str(), "P42");
    ASSERT_STREQ(p3.caption.c_str(), "P42");
    ASSERT_EQ(p3.arcsIn.size(), 0u);
    ASSERT_EQ(p3.arcsOut.size(), 0u);
    ASSERT_EQ(p1 == p3, true);
    ASSERT_EQ(p1 != p3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTransitionCreation)
{
    Transition t1(42u, 3.5f, 4.0f, 45u);
    ASSERT_EQ(t1.id, 42u);
    ASSERT_EQ(t1.type, Node::Transition);
    ASSERT_EQ(t1.angle, 45u);
    ASSERT_EQ(t1.x, 3.5f);
    ASSERT_EQ(t1.y, 4.0f);
    ASSERT_STREQ(t1.key.c_str(), "T42");
    ASSERT_STREQ(t1.caption.c_str(), "T42");
    ASSERT_EQ(t1.arcsIn.size(), 0u);
    ASSERT_EQ(t1.arcsOut.size(), 0u);
    ASSERT_EQ(t1.canFire(), false);
    ASSERT_EQ(t1.isInput(), false);
    ASSERT_EQ(t1.isOutput(), false);
    ASSERT_EQ(t1.isState(), false);

    Transition t2(t1);
    ASSERT_EQ(t2.id, 42u);
    ASSERT_EQ(t2.type, Node::Transition);
    ASSERT_EQ(t2.angle, 45u);
    ASSERT_EQ(t2.x, 3.5f);
    ASSERT_EQ(t2.y, 4.0f);
    ASSERT_STREQ(t2.key.c_str(), "T42");
    ASSERT_STREQ(t2.caption.c_str(), "T42");
    ASSERT_EQ(t2.arcsIn.size(), 0u);
    ASSERT_EQ(t2.arcsOut.size(), 0u);
    ASSERT_EQ(t2.canFire(), false);
    ASSERT_EQ(t2.isInput(), false);
    ASSERT_EQ(t2.isOutput(), false);
    ASSERT_EQ(t2.isState(), false);
    ASSERT_EQ(t1 == t2, true);
    ASSERT_EQ(t1 != t2, false);

    Transition t3(0u, 0.0f, 0.0f, 0u);
    ASSERT_EQ(t1 == t3, false);
    ASSERT_EQ(t1 != t3, true);

    t3 = t1;
    ASSERT_EQ(t3.id, 42u);
    ASSERT_EQ(t3.type, Node::Transition);
    ASSERT_EQ(t3.angle, 45u);
    ASSERT_EQ(t3.x, 3.5f);
    ASSERT_EQ(t3.y, 4.0f);
    ASSERT_STREQ(t3.key.c_str(), "T42");
    ASSERT_STREQ(t3.caption.c_str(), "T42");
    ASSERT_EQ(t3.arcsIn.size(), 0u);
    ASSERT_EQ(t3.arcsOut.size(), 0u);
    ASSERT_EQ(t3.canFire(), false);
    ASSERT_EQ(t3.isInput(), false);
    ASSERT_EQ(t3.isOutput(), false);
    ASSERT_EQ(t3.isState(), false);
    ASSERT_EQ(t1 == t3, true);
    ASSERT_EQ(t1 != t3, false);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestArcCreation)
{
    Transition t1(42u, 3.5f, 4.0f, 45u);
    Place p1(43u, 4.6f, 5.1f, 13u);

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
    ASSERT_EQ(reinterpret_cast<Transition&>(a1.from).angle, 45u);
    ASSERT_EQ(reinterpret_cast<Place&>(a1.to).tokens, 13u);
    ASSERT_EQ(a1.tokensOut(), 13u);

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
    ASSERT_EQ(reinterpret_cast<Transition&>(a2.to).angle, 45u);
    ASSERT_EQ(reinterpret_cast<Place&>(a2.from).tokens, 13u);
    ASSERT_EQ(a2.tokensIn(), 13u);

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
    ASSERT_EQ(reinterpret_cast<Transition&>(a3.from).angle, 45u);
    ASSERT_EQ(reinterpret_cast<Place&>(a3.to).tokens, 13u);
    ASSERT_EQ(a3.tokensOut(), 13u);

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
    ASSERT_EQ(reinterpret_cast<Transition&>(a4.from).angle, 45u);
    ASSERT_EQ(reinterpret_cast<Place&>(a4.to).tokens, 13u);
    ASSERT_EQ(a4.tokensOut(), 13u);

    ASSERT_EQ(a1 == a4, true);
    ASSERT_EQ(a1 != a4, false);
    ASSERT_EQ(a1 == a2, false);
    ASSERT_EQ(a1 != a2, true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestUtil)
{
    assert(Place::to_str(42u) == "P42");
    assert(Transition::to_str(0u) == "T0");
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, PetriNetDummy)
{
   PetriNet net;

   ASSERT_EQ(net.isEmpty(), true);
   ASSERT_EQ(net.isEventGraph(), false);
   ASSERT_EQ(net.m_next_place_id, 0u);
   ASSERT_EQ(net.m_next_transition_id, 0u);
   ASSERT_EQ(net.transitions().size(), 0u);
   ASSERT_EQ(net.places().size(), 0u);
   ASSERT_EQ(net.arcs().size(), 0u);
   ASSERT_EQ(&net.transitions(), &net.m_transitions);
   ASSERT_EQ(&net.places(), &net.m_places);
   ASSERT_EQ(&net.arcs(), &net.m_arcs);
   ASSERT_EQ(net.findNode("P0"), nullptr);
   ASSERT_EQ(net.findNode("T0"), nullptr);
   ASSERT_EQ(net.findNode("pouet"), nullptr);
   ASSERT_EQ(net.findNode(""), nullptr);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestAddInNet)
{
   PetriNet net;
   ASSERT_EQ(net.isEmpty(), true);
   ASSERT_EQ(net.m_next_place_id, 0u);
   ASSERT_EQ(net.m_next_transition_id, 0u);

   // Add Place 0: net = P0
   Place& p0 = net.addPlace(3.14f, 2.16f, 10u);
   ASSERT_EQ(net.m_next_place_id, 1u);
   ASSERT_EQ(p0.id, 0u);
   ASSERT_STREQ(p0.key.c_str(), "P0");
   ASSERT_EQ(net.isEmpty(), false);
   ASSERT_EQ(net.isEventGraph(), false);
   ASSERT_EQ(net.findNode("P0"), &p0);
   ASSERT_EQ(net.m_places.size(), 1u);
   ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");

   // Add Transition 0: net = P0 T0
   Transition* t0 = &net.addTransition(3.14f, 2.16f);
   ASSERT_EQ(net.m_next_transition_id, 1u);
   ASSERT_EQ(t0->id, 0u);
   ASSERT_STREQ(t0->key.c_str(), "T0");
   ASSERT_EQ(net.isEmpty(), false);
   ASSERT_EQ(net.isEventGraph(), false);
   ASSERT_EQ(net.findNode("T0"), t0);
   ASSERT_EQ(net.m_transitions.size(), 1u);
   ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");

   // Add Place 1: net = P0 T0 P1
   Place& p1 = net.addPlace(3.14f, 2.16f, 10u);
   ASSERT_EQ(net.m_next_place_id, 2u);
   ASSERT_EQ(p1.id, 1u);
   ASSERT_STREQ(p1.key.c_str(), "P1");
   ASSERT_EQ(net.findNode("P1"), &p1);
   ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
   ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");

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
   ASSERT_EQ(net.isEmpty(), false);
   ASSERT_EQ(net.isEventGraph(), false);
   ASSERT_EQ(net.findNode("T0"), t0);
   ASSERT_EQ(net.m_transitions.size(), 1u);
   ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");

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

   // Remove P1: net = P0 T0
   net.removeNode(p1);
   ASSERT_EQ(net.m_next_place_id, 1u);
   ASSERT_EQ(net.m_next_transition_id, 1u);
   ASSERT_EQ(net.m_arcs.size(), 0u);
   ASSERT_EQ(net.m_places.size(), 1u);
   ASSERT_EQ(net.m_transitions.size(), 1u);
   ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
   ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");

   // Remove P0: net = T0
   net.removeNode(p0);
   ASSERT_EQ(net.m_next_place_id, 0u);
   ASSERT_EQ(net.m_next_transition_id, 1u);
   ASSERT_EQ(net.m_arcs.size(), 0u);
   ASSERT_EQ(net.m_places.size(), 0u);
   ASSERT_EQ(net.m_transitions.size(), 1u);
   ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");

   // Remove T0: net
   net.removeNode(*t0);
   ASSERT_EQ(net.m_next_place_id, 0u);
   ASSERT_EQ(net.m_next_transition_id, 0u);
   ASSERT_EQ(net.m_arcs.size(), 0u);
   ASSERT_EQ(net.m_places.size(), 0u);
   ASSERT_EQ(net.m_transitions.size(), 0u);
   ASSERT_EQ(net.isEmpty(), true);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadJSON)
{
   PetriNet net;

   ASSERT_EQ(net.load("../examples/Howard2.json"), true); // FIXME shall call generateArcsInArcsOut ?
   net.generateArcsInArcsOut(); // FIXME

   ASSERT_EQ(net.isEmpty(), false);
   ASSERT_EQ(net.isEventGraph(), true);
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
   ASSERT_EQ(net.findNode("T0"), &net.m_transitions[0]);
   ASSERT_EQ(net.findNode("T1"), &net.m_transitions[1]);
   ASSERT_EQ(net.findNode("T2"), &net.m_transitions[2]);
   ASSERT_EQ(net.findNode("T3"), &net.m_transitions[3]);
   ASSERT_EQ(net.findNode("T4"), nullptr);
   ASSERT_EQ(net.findNode("pouet"), nullptr);
   ASSERT_EQ(net.findNode(""), nullptr);

   // Can we access to arcs ?

   // Can fire ? (Version 1)
   ASSERT_EQ(net.m_transitions[0].canFire(), 1u);
   ASSERT_EQ(net.m_transitions[1].canFire(), 0u);
   ASSERT_EQ(net.m_transitions[2].canFire(), 0u);
   ASSERT_EQ(net.m_transitions[3].canFire(), 0u);

   PetriNet canonic;
   net.toCanonicalForm(canonic); // FIXME shall return bool isEventGraph() ?
   canonic.generateArcsInArcsOut(); // FIXME

   ASSERT_EQ(canonic.isEmpty(), false);
   ASSERT_EQ(canonic.isEventGraph(), true);
   ASSERT_EQ(canonic.save("/tmp/canonic.json"), true);
   ASSERT_EQ(canonic.m_next_place_id, 6u);
   ASSERT_EQ(canonic.m_next_transition_id, 5u);
   ASSERT_EQ(canonic.m_places.size(), 6u);
   ASSERT_EQ(canonic.m_transitions.size(), 5u);
   ASSERT_EQ(canonic.m_arcs.size(), 12u);

   ASSERT_EQ(canonic.m_places[0].tokens, 1u);
   ASSERT_EQ(canonic.m_places[1].tokens, 0u);
   ASSERT_EQ(canonic.m_places[2].tokens, 0u);
   ASSERT_EQ(canonic.m_places[3].tokens, 0u);
   ASSERT_EQ(canonic.m_places[4].tokens, 0u);
   ASSERT_EQ(canonic.m_places[5].tokens, 1u);
}