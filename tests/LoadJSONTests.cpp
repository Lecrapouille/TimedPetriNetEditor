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

using namespace ::tpne;

//------------------------------------------------------------------------------
// New JSON format
TEST(TestJSONLoader, DummyTransitions)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "data/DummyTransitions.json", stringify).c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.m_places.size(), 1u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
// Identifiers of nodes are their indices: any order in the file, no gap.
TEST(TestJSONLoader, NodeIdentifiers)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "data/UnorderedIds.json", stringify).c_str(), "");
    ASSERT_EQ(net.m_places.size(), 2u);
    ASSERT_EQ(net.m_places[0].id, 0u);
    ASSERT_STREQ(net.m_places[0].caption.c_str(), "A");
    ASSERT_EQ(net.m_places[0].tokens, 2u);
    ASSERT_EQ(net.m_places[1].id, 1u);
    ASSERT_STREQ(net.m_places[1].caption.c_str(), "B");
    ASSERT_EQ(net.arcsOut(net.m_places[0]).size(), 1u);
    ASSERT_EQ(net.arcsIn(net.m_places[1]).size(), 1u);

    std::string const error = loadFromFile(net, "data/BadJSON/GapInIds.json", stringify);
    ASSERT_NE(error.find("identifiers of places shall be 0 .. 1"), std::string::npos) << error;
    ASSERT_EQ(net.isEmpty(), true);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, TestLoadedInvalidNetTimedPetri)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "doesnotexist", stringify).c_str(),
        "Cannot import 'doesnotexist'. Reason: 'unknown file extension'\n");
    ASSERT_EQ(net.isEmpty(), true);

    ASSERT_STREQ(loadFromFile(net, "doesnotexist.foo", stringify).c_str(),
        "Cannot import 'doesnotexist.foo'. Reason: 'unknown file extension'\n");
    ASSERT_EQ(net.isEmpty(), true);

    ASSERT_STREQ(loadFromFile(net, "data/BadJSON/BadType.json", stringify).c_str(),
        "Failed parsing 'data/BadJSON/BadType.json'. Reason: 'Unknown type of net: Timed event graphe'\n");
    ASSERT_EQ(net.isEmpty(), true);

    ASSERT_STREQ(loadFromFile(net, "data/BadJSON/NoName.json", stringify).c_str(),
    "Failed parsing 'data/BadJSON/NoName.json'. Reason: 'Missing JSON net name'\n");
    ASSERT_EQ(net.isEmpty(), true);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadJSONfile)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "data/GRAFCET.json", stringify).c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::GRAFCET);
    ASSERT_EQ(net.m_places.size(), 13u);
    ASSERT_EQ(net.m_transitions.size(), 11u);
    ASSERT_EQ(net.m_arcs.size(), 29u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadAsGrafcet)
{
    Net net(TypeOfNet::GRAFCET);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/TrafficLights.json", stringify).c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);
    ASSERT_EQ(net.m_places.size(), 6u);
    ASSERT_EQ(net.m_transitions.size(), 6u);
    ASSERT_EQ(net.m_arcs.size(), 16u);

    ASSERT_EQ(net.findPlace(0u)->tokens, 1u);
    ASSERT_EQ(net.findPlace(1u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(2u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(3u)->tokens, 0u);
    ASSERT_EQ(net.findPlace(4u)->tokens, 1u);
    ASSERT_EQ(net.findPlace(5u)->tokens, 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, CheckMarks)
{
    Net net(TypeOfNet::GRAFCET);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/TrafficLights.json", stringify).c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);

    std::vector<size_t> tokens;
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 6u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 0u);
    ASSERT_EQ(tokens[2], 0u);
    ASSERT_EQ(tokens[3], 0u);
    ASSERT_EQ(tokens[4], 1u);
    ASSERT_EQ(tokens[5], 0u);


    tokens[1] = 2u; tokens[2] = 3u; tokens[4] = 4u; tokens[5] = 5u;
    ASSERT_EQ(net.tokens(tokens), true);
    ASSERT_STREQ(net.error().c_str(), "");
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 6u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 2u);
    ASSERT_EQ(tokens[2], 3u);
    ASSERT_EQ(tokens[3], 0u);
    ASSERT_EQ(tokens[4], 4u);
    ASSERT_EQ(tokens[5], 5u);

    tokens.clear();
    ASSERT_EQ(net.tokens(tokens), false);
    ASSERT_STREQ(net.error().c_str(), "The container dimension holding tokens does not match the number of places\n");
    tokens = net.tokens();
    ASSERT_EQ(tokens.size(), 6u);
    ASSERT_EQ(tokens[0], 1u);
    ASSERT_EQ(tokens[1], 2u);
    ASSERT_EQ(tokens[2], 3u);
    ASSERT_EQ(tokens[3], 0u);
    ASSERT_EQ(tokens[4], 4u);
    ASSERT_EQ(tokens[5], 5u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, SaveAndLoadFile)
{
    std::string error;
    std::vector<size_t> erroneous_arcs;
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "../data/examples/EmergencyCalls.json", stringify).c_str(), "");
    convertTo(net, TypeOfNet::PetriNet, error, erroneous_arcs);
    ASSERT_STREQ(error.c_str(), "");
    ASSERT_EQ(erroneous_arcs.size(), 0u);
    ASSERT_STREQ(saveToFile(net, "/tmp/foo.json").c_str(), "");
    ASSERT_STREQ(loadFromFile(net,"/tmp/foo.json", stringify).c_str(), "");
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
    bool stringify;

    ASSERT_STREQ(saveToFile(net, "/tmp/foo.json").c_str(), "");
    net.addPlace(1.0, 1.0, 2u);
    ASSERT_EQ(net.m_places.size(), 1u);

    ASSERT_STREQ(loadFromFile(net, "/tmp/foo.json", stringify).c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.m_places.size(), 0u);
    ASSERT_EQ(net.m_transitions.size(), 0u);
    ASSERT_EQ(net.m_arcs.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(TestJSONLoader, LoadUnexistingFile)
{
    Net net(TypeOfNet::TimedPetriNet);
    bool stringify;

    ASSERT_STREQ(loadFromFile(net, "foooobar.json", stringify).c_str(),
    "Failed opening 'foooobar.json'. Reason: 'No such file or directory'\n");
}
