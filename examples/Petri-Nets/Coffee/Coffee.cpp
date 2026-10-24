// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The coffee machine run by the runtime: each transition of Coffee.json is a
// function of the machine. The coin checker (Insert Coin) returns Success for
// a good coin and Failed for a fake one: the token it puts in Coin Inserted
// has this color, and the arcs choose Accept (Success) or Reject Coin
// (Failed). A guard lets Insert Coin fire only while clients are waiting.
//   ./Coffee [Coffee.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Petri-Nets/Coffee/Coffee.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // The coins of the clients, in their order: the second one is fake, its
    // owner tries again with a good one.
    std::vector<bool> const coins = { true, false, true, true };
    size_t coin = 0u;
    int coffees = 0;

    Runtime runtime(net);
    runtime.on("Brew", []() { std::cout << "  heater: water is hot" << std::endl; });
    runtime.on("Insert Coin", [&coins, &coin]() {
        bool const good = coins[coin++];
        std::cout << "coin " << coin << ": " << (good ? "good" : "fake") << std::endl;
        return good; // true: Success, false: Failed
    });
    runtime.guard("Insert Coin", [&coins, &coin](Context const&) { return coin < coins.size(); });
    runtime.on("Reject Coin", []() { std::cout << "  coin given back" << std::endl; });
    runtime.on("Accept", []() { std::cout << "  coin accepted" << std::endl; });
    runtime.on("Dispense", [&coffees]() { std::cout << "  coffee " << ++coffees << " served" << std::endl; });

    // The machine never stops by itself: give the clients, then stop it.
    auto machine = runtime.instantiate("machine");
    double now = 0.0;
    while (runtime.step(*machine, now) && (now < 100.0) && ((coin < coins.size()) || (coffees < 3)))
        now += 1.0;
    runtime.cancel(*machine);
    runtime.run(*machine);
    std::cout << coffees << " coffees for " << coins.size() << " coins" << std::endl;
    return (coffees == 3) ? EXIT_SUCCESS : EXIT_FAILURE;
}
