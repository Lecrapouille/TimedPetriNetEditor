// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The philosophers run by the runtime on a pool of threads: a meal is the
// function of "PhiN eats", it lasts (it sleeps), and the philosophers run in
// parallel. The program has no mutex: the tokens of Fork and of the two
// Knife are the locks. Each pair of philosophers shares a cutlery, so the net
// lets one philosopher eat at a time: the program measures it. The three are
// hungry at the same time: the Random policy chooses who eats (the Priority
// one would choose Phi0, the first declared, every time). A guard gives
// each philosopher three meals; the program then stops the dinner (a guard
// can change: the instance waits for it, it does not end by itself).
//   ./Philosophers [Philosophers.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Petri-Nets/Philosophers/Philosophers.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // Philosophers eating now, the most at once, the meals of each one.
    std::atomic<int> eating{ 0 };
    std::atomic<int> most{ 0 };
    std::atomic<int> meals[3] = { 0, 0, 0 };
    std::mutex print; // only for the lines of the console
    auto const start = std::chrono::steady_clock::now();
    auto const ms = [&start]() {
        return int(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
    };

    // Three threads: the three philosophers could eat at once if the net let
    // them. A meal lasts 100 ms. The seed gives the same choices at each run.
    Runtime::Options options;
    options.policy = Engine::Policy::Random;
    options.seed = 1u;
    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(3u), options);
    for (int i = 0; i < 3; ++i)
    {
        std::string const name = "Phi" + std::to_string(i);
        runtime.on(name + " eats", [&, i, name]() {
            int const now = ++eating;
            int seen = most.load();
            while ((now > seen) && !most.compare_exchange_weak(seen, now)) {}
            {
                std::lock_guard<std::mutex> lock(print);
                std::printf("%4d ms  %s starts meal %d (%d eating)\n", ms(), name.c_str(), meals[i].load() + 1, now);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            --eating;
        });
        runtime.on(name + " thinks", [&, i]() { ++meals[i]; }); // the cutlery goes back
        runtime.guard(name + " eats", [&meals, i](Context const&) { return meals[i] < 3; });
    }

    // The loop of the program: step() fires what can fire and gives the
    // tokens of the functions that ended.
    auto dinner = runtime.instantiate("dinner");
    while (runtime.step(*dinner) && ((meals[0] + meals[1] + meals[2]) < 9))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    runtime.cancel(*dinner);
    Color const result = runtime.run(*dinner);
    std::printf("%4d ms  %s: %d, %d and %d meals, at most %d philosopher eating at once\n", ms(),
                net.colors.name(result).c_str(), meals[0].load(), meals[1].load(), meals[2].load(), most.load());
    return ((most == 1) && (meals[0] == 3) && (meals[1] == 3) && (meals[2] == 3)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
