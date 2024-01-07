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
#  include "TimedPetriNetEditor/PetriNet.hpp"
#undef protected
#undef private

using namespace ::tpne;

//------------------------------------------------------------------------------
// New JSON format
TEST(TestJSONLoader, DummyTransitions)
{
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net,"data/DummyTransitions.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, TestLoadedInvalidNetTimedPetri)
{
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net,"doesnotexist").c_str(),
        "Failed opening 'doesnotexist'."
        " Reason was 'No such file or directory'\n");
    ASSERT_EQ(net.isEmpty(), true);

    ASSERT_STREQ(loadFromFile(net,"data/BadJSON/BadType.json").c_str(),
        "Failed parsing 'data/BadJSON/BadType.json'."
        " Reason was 'Unknown type of net: Timed event graphe'\n");
    ASSERT_EQ(net.isEmpty(), true);

    ASSERT_STREQ(loadFromFile(net,"data/BadJSON/NoName.json").c_str(),
    "Failed parsing 'data/BadJSON/NoName.json'."
    " Reason was 'Missing JSON net name'\n");
    ASSERT_EQ(net.isEmpty(), true);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadJSONfile)
{
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net,"data/GRAFCET.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::GRAFCET);
    ASSERT_EQ(net.m_places.size(), 13u);
    ASSERT_EQ(net.m_transitions.size(), 11u);
    ASSERT_EQ(net.m_arcs.size(), 29u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadAsGrafcet)
{
    Net net(TypeOfNet::GRAFCET);

    ASSERT_STREQ(loadFromFile(net,"data/TrafficLights.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
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
TEST(TestJSONLoader, CheckMarks)
{
    Net net(TypeOfNet::GRAFCET);
    ASSERT_STREQ(loadFromFile(net,"data/TrafficLights.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);

    std::vector<size_t> tokens;
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 7u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 0u);
    ASSERT_EQ(tokens[2], 0u);
    ASSERT_EQ(tokens[3], 1u);
    ASSERT_EQ(tokens[4], 0u);
    ASSERT_EQ(tokens[5], 0u);
    ASSERT_EQ(tokens[6], 1u);

    tokens[1] = 2u; tokens[2] = 3u; tokens[4] = 4u; tokens[5] = 5u;
    ASSERT_EQ(net.tokens(tokens), true);
    ASSERT_STREQ(net.error().c_str(), "");
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 7u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 2u);
    ASSERT_EQ(tokens[2], 3u);
    ASSERT_EQ(tokens[3], 1u);
    ASSERT_EQ(tokens[4], 4u);
    ASSERT_EQ(tokens[5], 5u);
    ASSERT_EQ(tokens[6], 1u);

    tokens.clear();
    ASSERT_EQ(net.tokens(tokens), false);
    ASSERT_STREQ(net.error().c_str(), "The container dimension holding tokens does not match the number of places\n");
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 7u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 2u);
    ASSERT_EQ(tokens[2], 3u);
    ASSERT_EQ(tokens[3], 1u);
    ASSERT_EQ(tokens[4], 4u);
    ASSERT_EQ(tokens[5], 5u);
    ASSERT_EQ(tokens[6], 1u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, SaveAndLoadFile)
{
    std::string error;
    std::vector<Arc*> erroneous_arcs;
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net,"data/AppelsDurgence.json").c_str(), "");
    convertTo(net, TypeOfNet::PetriNet, error, erroneous_arcs);
    ASSERT_STREQ(error.c_str(), "");
    ASSERT_EQ(erroneous_arcs.size(), 0u);
    ASSERT_STREQ(saveToFile(net, "/tmp/foo.json").c_str(), "");
    ASSERT_STREQ(loadFromFile(net,"/tmp/foo.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);
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
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(saveToFile(net, "/tmp/foo.json").c_str(), "");
    net.addPlace(1.0, 1.0, 2u);
    ASSERT_EQ(net.m_places.size(), 1u);

    ASSERT_STREQ(loadFromFile(net,"/tmp/foo.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadUnexistingFile)
{
    Net net(TypeOfNet::TimedPetriNet);
    ASSERT_STREQ(loadFromFile(net,"foooobar.json").c_str(),
    "Failed opening 'foooobar.json'. Reason was 'No such file or directory'\n");
}
