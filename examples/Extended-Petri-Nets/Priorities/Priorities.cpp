// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Conflicts solved by the runtime: the car and the ambulance want the same
// crossing at the same date. Options::policy chooses: Priority (the default)
// lets the transition of highest priority fire (the ambulance, priority 1 in
// the net), Random draws one, with a seed that gives the same draws again.
// The net is a timed Petri net: run on a simulated clock (step() with the
// date), its durations are waited without sleeping.
//   ./Priorities [Priorities.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace tpne;

//------------------------------------------------------------------------------
//! \brief The vehicles crossing in the first 20 time units, in their order.
static std::vector<std::string> crossings(Net const& net, Runtime::Options const& options, bool const verbose)
{
    std::vector<std::string> order;
    double now = 0.0;
    Runtime runtime(net, std::make_shared<InlineExecutor>(), options);
    for (char const* vehicle : { "Car", "Ambulance" })
    {
        std::string const name(vehicle);
        runtime.on(name + " crosses", [&order, &now, name, verbose]() {
            order.push_back(name);
            if (verbose)
                std::printf("  t = %4.1f  %s crosses\n", now, name.c_str());
        });
    }
    auto crossing = runtime.instantiate("crossing");
    while ((now <= 20.0) && runtime.step(*crossing, now))
        now += 0.5;
    return order;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Extended-Petri-Nets/Priorities/Priorities.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // Priority: the ambulance first, then they alternate as they come back.
    Runtime::Options options;
    options.log = false;
    options.policy = Engine::Policy::Priority;
    std::cout << "Policy Priority:" << std::endl;
    std::vector<std::string> const by_priority = crossings(net, options, true);

    // Random: who crosses first at t = 0 depends on the seed only.
    options.policy = Engine::Policy::Random;
    int cars = 0;
    std::cout << "Policy Random, first to cross for the seeds 1 to 10:" << std::endl << " ";
    for (uint64_t seed = 1u; seed <= 100u; ++seed)
    {
        options.seed = seed;
        std::string const first = crossings(net, options, false).front();
        cars += (first == "Car") ? 1 : 0;
        if (seed <= 10u)
            std::cout << " " << first;
    }
    std::cout << std::endl << "  the car first " << cars << " times for the seeds 1 to 100" << std::endl;

    // The same seed, the same run.
    options.seed = 7u;
    bool const same = (crossings(net, options, false) == crossings(net, options, false));
    std::cout << "Seed 7 twice: " << (same ? "the same crossings" : "different crossings") << std::endl;

    return ((by_priority.front() == "Ambulance") && same) ? EXIT_SUCCESS : EXIT_FAILURE;
}
