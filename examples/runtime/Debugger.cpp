// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The editor as the debugger of a program: the pick and place of
// PickAndPlace.cpp, over and over, publishing its journal on ZeroMQ. Open
// data/examples/PickAndPlace.json in the editor, then Debug > Connect
// (tcp://localhost:5556): the Journal follows the orders, the net shows their
// tokens, retry is marked as a transition without function.
//   ./Debugger [seconds]        (default: 30 s)

#include "IO/IO.hpp"
#include "PetriNet/PetriNet.hpp"
#include "Runtime/JournalPublisher.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>

using namespace tpne;

int main(int argc, char* argv[])
{
    double const seconds = (argc > 1) ? std::atof(argv[1]) : 30.0;
    Net net;
    std::string const error = loadFromFile(net, "data/examples/PickAndPlace.json");
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    JournalPublisher publisher(net);
    if (!publisher.error().empty())
    {
        std::cerr << publisher.error() << std::endl;
        return EXIT_FAILURE;
    }
    Runtime::Options options;
    options.journal = [&publisher](LogEntry const& entry) { publisher.publish(entry); };
    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(2u), options);

    // Slow functions, so that the tokens can be seen moving; grasp fails
    // one time out of three.
    std::mt19937 random(42u);
    runtime.on("detect", []() { std::this_thread::sleep_for(std::chrono::milliseconds(700)); });
    runtime.on("grasp", [&random]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(900));
        return (random() % 3u) != 0u;
    });
    runtime.on("place", []() { std::this_thread::sleep_for(std::chrono::milliseconds(600)); });
    publisher.manifest(runtime);
    std::cout << "Publishing on tcp://*:5556 during " << seconds << " s" << std::endl;

    auto const end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    for (int order = 1; std::chrono::steady_clock::now() < end; ++order)
    {
        auto instance = runtime.instantiate("order_" + std::to_string(order), {}, { { "Done", Success } });
        publisher.marking(*instance);
        while (runtime.step(*instance) && (std::chrono::steady_clock::now() < end))
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        publisher.marking(*instance);
        std::cout << instance->id() << ": " << net.colors.name(instance->result()) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return EXIT_SUCCESS;
}
