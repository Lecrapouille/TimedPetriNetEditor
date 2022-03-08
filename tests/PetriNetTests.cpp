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
    ASSERT_EQ(Place::s_next_id, 0u);

    Place p0(3.5f, 4.0f);
    Place p1(3.5f, 4.0f, 12u);
    ASSERT_EQ(Place::s_next_id, 2u);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestLoadJSON)
{
   PetriNet net;

   ASSERT_EQ(net.load("../examples/Howard2.json"), true); // FIXME shall call generateArcsInArcsOut ?
   //net.generateArcsInArcsOut();
   ASSERT_EQ(Place::s_next_id, 5u);
   ASSERT_EQ(Transition::s_next_id, 4u);
   ASSERT_EQ(net.m_places.size(), 5u);
   ASSERT_EQ(net.m_transitions.size(), 4u);
   ASSERT_EQ(net.m_arcs.size(), 10u);

   ASSERT_EQ(net.m_places[0].id, 0u);
   ASSERT_STREQ(net.m_places[0].key.c_str(), "P0");
   ASSERT_STREQ(net.m_places[0].caption.c_str(), "P0");
   ASSERT_EQ(net.m_places[0].type, Node::Place);
   ASSERT_EQ(net.m_places[0].tokens, 2u);
   ASSERT_EQ(net.m_places[0].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_places[0].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_places[1].id, 1u);
   ASSERT_STREQ(net.m_places[1].key.c_str(), "P1");
   ASSERT_STREQ(net.m_places[1].caption.c_str(), "P1");
   ASSERT_EQ(net.m_places[1].type, Node::Place);
   ASSERT_EQ(net.m_places[1].tokens, 0u);
   ASSERT_EQ(net.m_places[1].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_places[1].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_places[2].id, 2u);
   ASSERT_STREQ(net.m_places[2].key.c_str(), "P2");
   ASSERT_STREQ(net.m_places[2].caption.c_str(), "P2");
   ASSERT_EQ(net.m_places[2].type, Node::Place);
   ASSERT_EQ(net.m_places[2].tokens, 0u);
   ASSERT_EQ(net.m_places[2].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_places[2].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_places[3].id, 3u);
   ASSERT_STREQ(net.m_places[3].key.c_str(), "P3");
   ASSERT_STREQ(net.m_places[3].caption.c_str(), "P3");
   ASSERT_EQ(net.m_places[3].type, Node::Place);
   ASSERT_EQ(net.m_places[3].tokens, 0u);
   ASSERT_EQ(net.m_places[3].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_places[3].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_places[4].id, 4u);
   ASSERT_STREQ(net.m_places[4].key.c_str(), "P4");
   ASSERT_STREQ(net.m_places[4].caption.c_str(), "P4");
   ASSERT_EQ(net.m_places[4].type, Node::Place);
   ASSERT_EQ(net.m_places[4].tokens, 0u);
   ASSERT_EQ(net.m_places[4].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_places[4].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_transitions[0].id, 0u);
   ASSERT_STREQ(net.m_transitions[0].key.c_str(), "T0");
   ASSERT_STREQ(net.m_transitions[0].caption.c_str(), "T0");
   ASSERT_EQ(net.m_transitions[0].type, Node::Transition);
   ASSERT_EQ(net.m_transitions[0].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_transitions[0].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_transitions[1].id, 1u);
   ASSERT_STREQ(net.m_transitions[1].key.c_str(), "T1");
   ASSERT_STREQ(net.m_transitions[1].caption.c_str(), "T1");
   ASSERT_EQ(net.m_transitions[1].type, Node::Transition);
   ASSERT_EQ(net.m_transitions[1].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_transitions[1].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_transitions[2].id, 2u);
   ASSERT_STREQ(net.m_transitions[2].key.c_str(), "T2");
   ASSERT_STREQ(net.m_transitions[2].caption.c_str(), "T2");
   ASSERT_EQ(net.m_transitions[2].type, Node::Transition);
   ASSERT_EQ(net.m_transitions[2].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_transitions[2].arcsOut.size(), 0u);

   ASSERT_EQ(net.m_transitions[3].id, 3u);
   ASSERT_STREQ(net.m_transitions[3].key.c_str(), "T3");
   ASSERT_STREQ(net.m_transitions[3].caption.c_str(), "T3");
   ASSERT_EQ(net.m_transitions[3].type, Node::Transition);
   ASSERT_EQ(net.m_transitions[3].arcsIn.size(), 0u);
   ASSERT_EQ(net.m_transitions[3].arcsOut.size(), 0u);

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

   // ... a finir

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

   ASSERT_EQ(net.isEventGraph(), true);
}
