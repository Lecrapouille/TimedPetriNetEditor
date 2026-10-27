// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"
#include "MaxPlus/Algorithms.hpp"
#include "PetriNet/Simulation.hpp"
#include "PetriNet/ColoredMarking.hpp"

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief Load the net Stock -(2)-> Move -(3)-> Buffer (capacity 4), with an
//! inhibitor arc from Alarm and a reset arc from Trash.
static void loadExtensions(Net& net)
{
    bool springify;
    ASSERT_STREQ(loadFromFile(net, "data/Extensions.json", springify).c_str(), "");
}

//------------------------------------------------------------------------------
TEST(TestExtensions, LoadAndFeatures)
{
    Net net(TypeOfNet::PetriNet);
    loadExtensions(net);

    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);
    ASSERT_EQ(net.arcs().size(), 4u);
    Arc const* stock = net.findArc(net.places()[0], net.transitions()[0]);
    Arc const* alarm = net.findArc(net.places()[1], net.transitions()[0]);
    Arc const* trash = net.findArc(net.places()[3], net.transitions()[0]);
    Arc const* buffer = net.findArc(net.transitions()[0], net.places()[2]);
    ASSERT_TRUE((stock != nullptr) && (alarm != nullptr) && (trash != nullptr) && (buffer != nullptr));
    ASSERT_EQ(stock->kind, ArcKind::Normal);
    ASSERT_EQ(stock->weight, 2u);
    ASSERT_EQ(alarm->kind, ArcKind::Inhibitor);
    ASSERT_EQ(alarm->weight, 1u);
    ASSERT_EQ(trash->kind, ArcKind::Reset);
    ASSERT_EQ(buffer->weight, 3u);
    ASSERT_EQ(net.places()[2].capacity, 4u);

    unsigned const all = Feature::Weights | Feature::Inhibitors | Feature::Resets | Feature::Capacities;
    ASSERT_EQ(features(net), all);
    ASSERT_TRUE(allows(TypeOfNet::PetriNet, all));
    ASSERT_TRUE(allows(TypeOfNet::TimedPetriNet, all));
    ASSERT_FALSE(allows(TypeOfNet::GRAFCET, Feature::Inhibitors));
    ASSERT_STREQ(featuresToStr(Feature::Weights | Feature::Resets).c_str(), "weights, reset arcs");
}

//------------------------------------------------------------------------------
TEST(TestExtensions, EnablingRules)
{
    Net net(TypeOfNet::PetriNet);
    loadExtensions(net);
    Transition const& move = net.transitions()[0];

    // Stock 3 >= 2, Alarm 0 < 1, Buffer 0 + 3 <= 4
    ASSERT_TRUE(net.isEnabled(move));

    // Not enough tokens for the weight
    net.places()[0].tokens = 1u;
    ASSERT_FALSE(net.isEnabled(move));
    net.places()[0].tokens = 2u;
    ASSERT_TRUE(net.isEnabled(move));

    // The inhibitor arc forbids when the alarm holds a token
    net.places()[1].tokens = 1u;
    ASSERT_FALSE(net.isEnabled(move));
    net.places()[1].tokens = 0u;

    // The reset arc does not need tokens
    net.places()[3].tokens = 0u;
    ASSERT_TRUE(net.isEnabled(move));

    // The buffer would hold more than its capacity
    net.places()[2].tokens = 2u;
    ASSERT_FALSE(net.isEnabled(move));
    net.places()[2].tokens = 1u;
    ASSERT_TRUE(net.isEnabled(move));
}

//------------------------------------------------------------------------------
TEST(TestExtensions, Firing)
{
    Net net(TypeOfNet::PetriNet);
    loadExtensions(net);

    // Petri net: the user arms the transition by a click.
    Simulation simulation(net);
    simulation.start();
    simulation.step(0.0f); // starts
    net.transitions()[0].receptivity = true;
    simulation.step(0.01f); // fires
    ASSERT_EQ(net.places()[0].tokens, 1u); // 3 - 2
    ASSERT_EQ(net.places()[1].tokens, 0u); // inhibitor: unchanged
    ASSERT_EQ(net.places()[3].tokens, 0u); // reset: emptied
    for (int i = 0; i < 50; ++i)
        simulation.step(0.01f); // tokens on their way
    ASSERT_EQ(net.places()[2].tokens, 3u);
    simulation.stop();
    simulation.step(0.01f); // halting
    simulation.step(0.01f); // restores

    // The initial marking is restored after the simulation.
    ASSERT_EQ(net.places()[0].tokens, 3u);
    ASSERT_EQ(net.places()[3].tokens, 5u);
}

//------------------------------------------------------------------------------
TEST(TestExtensions, SaveAndLoad)
{
    Net net(TypeOfNet::PetriNet);
    loadExtensions(net);
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-extensions.json").c_str(), "");

    Net loaded(TypeOfNet::PetriNet);
    bool springify;
    ASSERT_STREQ(loadFromFile(loaded, "/tmp/tpne-extensions.json", springify).c_str(), "");
    ASSERT_EQ(features(loaded), features(net));
    for (size_t i = 0u; i < net.arcs().size(); ++i)
    {
        ASSERT_EQ(loaded.arcs()[i].kind, net.arcs()[i].kind);
        ASSERT_EQ(loaded.arcs()[i].weight, net.arcs()[i].weight);
    }
    ASSERT_EQ(loaded.places()[2].capacity, 4u);
    ASSERT_EQ(loaded.places()[0].capacity, Net::defaultCapacity(TypeOfNet::PetriNet));
}

//------------------------------------------------------------------------------
TEST(TestExtensions, Conversions)
{
    Net net(TypeOfNet::PetriNet);
    loadExtensions(net);
    std::string error;
    std::vector<size_t> arcs;

    // Event graphs and GRAFCET refuse the extensions: the arcs are given.
    ASSERT_FALSE(convertToEventGraph(net, error, arcs));
    ASSERT_NE(error.find("weights"), std::string::npos) << error;
    ASSERT_EQ(arcs.size(), 4u);
    ASSERT_FALSE(convertTo(net, TypeOfNet::GRAFCET, error, arcs));
    ASSERT_EQ(net.type(), TypeOfNet::PetriNet);
    ASSERT_FALSE(isEventGraph(net, error, arcs));

    // Timed Petri nets accept them and keep the capacities.
    ASSERT_TRUE(convertTo(net, TypeOfNet::TimedPetriNet, error, arcs));
    ASSERT_EQ(net.places()[2].capacity, 4u);
}

//------------------------------------------------------------------------------
TEST(TestExtensions, ImportRefusesExtensionsOfGrafcet)
{
    Net net(TypeOfNet::PetriNet);
    bool springify;
    std::string const error = loadFromFile(net, "data/BadJSON/GrafcetWeights.json", springify);
    ASSERT_NE(error.find("A GRAFCET cannot have weights"), std::string::npos) << error;
}

//------------------------------------------------------------------------------
TEST(TestExtensions, Colors)
{
    ColorTable table;
    ASSERT_EQ(table.add("Success"), Success);
    ASSERT_EQ(table.add("Red"), Color(5u));
    ASSERT_EQ(table.add("Red"), Color(5u));
    ASSERT_EQ(table.find("Blue"), std::nullopt);
    ASSERT_STREQ(table.name(Failed).c_str(), "Failed");
    ASSERT_EQ(table.user(), std::vector<std::string>({ "Red" }));

    // Grasp -> Held; Held -(Success)-> Place; Held -(Failed)-> Retry
    Net net(TypeOfNet::PetriNet);
    Place& held = net.addPlace(0u);
    Transition& place = net.addTransition();
    Transition& retry = net.addTransition();
    net.addArc(held, place);
    net.addArc(held, retry);
    net.findArc(held, place)->color = Success;
    net.findArc(held, retry)->color = net.colors.add("Dropped");
    ASSERT_EQ(features(net), unsigned(Feature::Colors));
    ASSERT_TRUE(allows(TypeOfNet::PetriNet, Feature::Colors));
    ASSERT_FALSE(allows(TypeOfNet::GRAFCET, Feature::Colors));

    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-colors.json").c_str(), "");
    Net loaded(TypeOfNet::PetriNet);
    bool springify;
    ASSERT_STREQ(loadFromFile(loaded, "/tmp/tpne-colors.json", springify).c_str(), "");
    ASSERT_EQ(loaded.colors.user(), std::vector<std::string>({ "Dropped" }));
    ASSERT_EQ(loaded.findArc(loaded.places()[0], loaded.transitions()[0])->color, Success);
    ASSERT_STREQ(loaded.colors.name(loaded.findArc(loaded.places()[0], loaded.transitions()[1])->color).c_str(),
                 "Dropped");
}

//------------------------------------------------------------------------------
TEST(TestExtensions, ColoredInitialTokens)
{
    Net net(TypeOfNet::PetriNet);
    bool springify;
    ASSERT_STREQ(loadFromFile(net, "../examples/Extended-Petri-Nets/ColoredPhilosophers/ColoredPhilosophers.json",
                              springify).c_str(), "");
    std::optional<Color> const phi1 = net.colors.find("Phi1");
    ASSERT_TRUE(phi1.has_value());

    // Thinking holds Phi0, Phi1, Phi2; the forks are Success.
    ColoredMarking const marking = initialMarking(net);
    ASSERT_EQ(marking.count(0u), 3u);
    ASSERT_EQ(marking.count(0u, *phi1), 1u);
    ASSERT_EQ(marking.count(0u, Success), 0u);
    ASSERT_EQ(marking.count(2u, Success), 1u);

    // Saved and loaded again.
    ASSERT_STREQ(saveToFile(net, "/tmp/tpne-colored-tokens.json").c_str(), "");
    Net loaded(TypeOfNet::PetriNet);
    ASSERT_STREQ(loadFromFile(loaded, "/tmp/tpne-colored-tokens.json", springify).c_str(), "");
    ASSERT_TRUE(initialMarking(loaded) == marking);

    // Fewer tokens than colored ones: the first colors are kept.
    net.places()[0].tokens = 1u;
    ASSERT_EQ(initialMarking(net).count(0u), 1u);
    ASSERT_EQ(initialMarking(net).count(0u, *net.colors.find("Phi0")), 1u);
}
