//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
// New JSON format
TEST(TestJSONLoader, DummyTransitions)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/DummyTransitions.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadJSONfile)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/GRAFCET.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::GRAFCET);
    ASSERT_EQ(net.m_places.size(), 13u);
    ASSERT_EQ(net.m_transitions.size(), 11u);
    ASSERT_EQ(net.m_arcs.size(), 29u);
}

//------------------------------------------------------------------------------
// Compared to TEST(TestJSONLoader, WithCarriageReturn) places have max 1 token.
TEST(TestJSONLoader, LoadAsGrafcet)
{
    PetriNet net(PetriNet::Type::GRAFCET);

    ASSERT_EQ(net.load("data/TrafficLights.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.m_places.size(), 7u);
    ASSERT_EQ(net.m_transitions.size(), 6u);
    ASSERT_EQ(net.m_arcs.size(), 16u);

    ASSERT_EQ(net.findPlace(0u)->tokens, 1u);
    ASSERT_EQ(net.findPlace(3u)->tokens, 1u);
    ASSERT_EQ(net.findPlace(6u)->tokens, 1u);

    ASSERT_EQ(net.findPlace(1u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(2u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(4u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(5u)->tokens, 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, SaveAndLoadFile)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.load("data/AppelsDurgence.json"), true);
    net.type(PetriNet::Type::Petri);
    ASSERT_EQ(net.save("/tmp/foo.json"), true);
    ASSERT_EQ(net.load("/tmp/foo.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::Petri);
    ASSERT_EQ(net.m_places.size(), 13u);
    ASSERT_EQ(net.m_transitions.size(), 11u);
    ASSERT_EQ(net.m_arcs.size(), 29u);
    ASSERT_EQ(net.findPlace(4u)->tokens, 4u);
    ASSERT_EQ(net.findPlace(9u)->tokens, 4u);
    ASSERT_EQ(net.findPlace(10u)->tokens, 7u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, SaveAndLoadDummyNet)
{
    PetriNet net(PetriNet::Type::TimedPetri);

    ASSERT_EQ(net.save("/tmp/foo.json"), true);
    net.addPlace(1.0, 1.0, 2u);
    ASSERT_EQ(net.m_places.size(), 1u);

    ASSERT_EQ(net.load("/tmp/foo.json"), true);
    ASSERT_EQ(net.type(), PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadUnexistingFile)
{
    PetriNet net(PetriNet::Type::TimedPetri);
    ASSERT_EQ(net.load("foooobar.json"), false);
}
