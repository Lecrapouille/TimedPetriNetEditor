// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The colored philosophers run twice: by the engine of the editor (the
// simulation) and by the runtime. Both follow the colors with the same rule
// (PetriNet/Firing.hpp): "Phi0 eats" takes only the token Phi0 of Thinking.
// With the same policy they fire the same transitions in the same order: the
// program compares the two sequences and prints the colored marking.
//   ./ColoredPhilosophers [ColoredPhilosophers.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace tpne;

static constexpr size_t FIRINGS = 12u;

//------------------------------------------------------------------------------
//! \brief "Thinking: Phi0 Phi1 | Eating: Phi2 | ..." (places with tokens).
static std::string show(Net const& net, ColoredMarking const& marking)
{
    std::string text;
    for (auto const& place : net.places())
    {
        std::vector<std::pair<Color, size_t>> const tokens = marking.tokens(place.id);
        if (tokens.empty())
            continue;
        text += (text.empty() ? "" : " | ") + place.caption + ":";
        for (auto const& [color, n] : tokens)
        {
            for (size_t i = 0u; i < n; ++i)
                text += " " + net.colors.name(color);
        }
    }
    return text;
}

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1]
                                  : "examples/Extended-Petri-Nets/ColoredPhilosophers/ColoredPhilosophers.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    std::printf("initial    %s\n", show(net, initialMarking(net)).c_str());

    // The simulation of the editor: the tokens arrive at once, the first
    // declared transition fires first.
    std::vector<std::string> simulated;
    Engine::Options options;
    options.policy = Engine::Policy::FirstDeclared;
    options.timed = false;
    options.max_firings_per_date = FIRINGS;
    Engine engine(net, options);
    engine.onFire = [&](size_t t, double, size_t) {
        simulated.push_back(net.transitions()[t].caption);
        // The tokens given are on their way (they arrive at once after).
        std::string going;
        for (auto const& flight : engine.flights())
        {
            Arc const& arc = net.arcs()[flight.arc];
            going += " " + net.places()[arc.to.id].caption + "<-" + net.colors.name(flight.color);
        }
        std::printf("simulation %-12s %s | going:%s\n", simulated.back().c_str(),
                    show(net, engine.coloredMarking()).c_str(), going.c_str());
    };
    engine.onWarning = [](std::string const&) {}; // the dinner never ends
    engine.reset();
    engine.settle();

    // The runtime, with the same policy: functions without duration, a guard
    // stops the dinner after the same number of firings.
    std::vector<std::string> run;
    Runtime::Options rt;
    rt.policy = Engine::Policy::FirstDeclared;
    rt.journal = [&run](LogEntry const& entry) {
        if (entry.event == LogEntry::Event::Scheduled)
            run.push_back(entry.transition);
    };
    Runtime runtime(net, std::make_shared<InlineExecutor>(), rt);
    for (auto const& t : net.transitions())
        runtime.guard(t.caption, [&run](Context const&) { return run.size() < FIRINGS; });
    auto dinner = runtime.instantiate("dinner");
    while (runtime.step(*dinner) && (run.size() < FIRINGS)) {}
    std::printf("runtime    %s\n", show(net, dinner->marking()).c_str());
    runtime.cancel(*dinner);

    bool const same = (simulated == run);
    std::printf("%zu firings, simulation and runtime %s\n", run.size(), same ? "agree" : "DIFFER");
    return (same && (run.size() == FIRINGS)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
