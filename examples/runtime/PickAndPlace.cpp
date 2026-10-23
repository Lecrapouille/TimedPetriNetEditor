// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: example of the runtime (desktop, pool of threads).
//
// The net data/examples/PickAndPlace.json is drawn in the editor; this
// program gives a function to its transitions. grasp fails twice (Failed):
// the colors asked by the arcs from Held send the token to retry, then to
// place when grasp succeeds.
//=============================================================================

#include "PetriNet/PetriNet.hpp"
#include "Runtime/Runtime.hpp"
#include "Runtime/Journal.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "data/examples/PickAndPlace.json";
    Net net;
    bool springify;
    std::string const error = loadFromFile(net, file, springify);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(4u));
    std::atomic<int> attempts{0};
    runtime.on("detect", []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "camera: object seen" << std::endl;
    });
    runtime.on("grasp", [&attempts]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool const held = (++attempts >= 3);
        std::cout << "gripper: " << (held ? "holding" : "slipped") << std::endl;
        return held;
    });
    runtime.on("place", []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        std::cout << "arm: placed" << std::endl;
    });
    for (auto const& name : runtime.unregisteredTransitions())
        std::cout << "no function (fires at once): " << name << std::endl;

    auto order = runtime.instantiate("order_42", {}, { { "Done", Success } });
    Color const result = runtime.run(*order);
    std::cout << "order_42: " << net.colors.name(result) << std::endl << std::endl
              << toJsonLines(order->log(), net.colors);
    return (result == Success) ? EXIT_SUCCESS : EXIT_FAILURE;
}
