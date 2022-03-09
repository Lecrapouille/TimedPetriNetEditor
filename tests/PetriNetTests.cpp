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
