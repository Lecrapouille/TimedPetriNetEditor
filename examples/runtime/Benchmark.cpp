// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: benchmark of the runtime, against Symmetri.
//
// The workload of the example "performance" of Symmetri
// (https://github.com/thorstink/Symmetri/tree/main/examples/performance):
// without argument the net t0: Pa + Pb -> Pc, t1: 2 Pc -> 2 Pb, 4 tokens in
// Pa and 2 in Pb, goal 2 Pb + 2 Pc, synchronous functions; with a PNML file
// (the nets of Symmetri), its transitions without function until nothing
// can fire. One run is timed as Symmetri does, then the mean of many runs.
//
//   ./Benchmark [net.pnml] [runs]
//
// Symmetri: build its examples (cmake -DBUILD_EXAMPLES=ON) and run
// ./Performance [net.pnml] on the same machine.
//=============================================================================

#include "IO/IO.hpp"
#include "PetriNet/PetriNet.hpp"
#include "Runtime/Runtime.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace tpne;

namespace {

//! \brief A function run in the thread of run() (as isSynchronous() of
//! Symmetri), counting the firings.
size_t g_firings = 0u;

struct Simple {};
bool isSynchronous(Simple const&) { return true; }
Color fire(Simple const&)
{
    ++g_firings;
    return Success;
}

//------------------------------------------------------------------------------
Net makeNet()
{
    Net net(TypeOfNet::PetriNet);
    net.name = "benchmark";
    Place& pa = net.addPlace(4u, "Pa");
    Place& pb = net.addPlace(2u, "Pb");
    Place& pc = net.addPlace(0u, "Pc");
    Transition& t0 = net.addTransition("t0");
    Transition& t1 = net.addTransition("t1");
    net.addArc(pa, t0);
    net.addArc(pb, t0);
    net.addArc(t0, pc);
    net.addArc(pc, t1);
    net.addArc(t1, pb);
    net.findArc(pc, t1)->weight = 2u;
    net.findArc(t1, pb)->weight = 2u;
    return net;
}

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Net net;
    std::vector<PlaceTokens> goal;
    bool const file = (argc > 1);
    if (file)
    {
        std::string const error = loadFromFile(net, argv[1]);
        if (!error.empty())
        {
            std::cerr << error;
            return EXIT_FAILURE;
        }
    }
    else
    {
        net = makeNet();
        goal = { { "Pb", Success, 2u }, { "Pc", Success, 2u } };
    }
    size_t const runs = (argc > 2) ? std::strtoul(argv[2], nullptr, 10) : 10000u;

    // A transition without input place waits for inject() (a message): run()
    // would wait for ever, as a server does.
    for (auto const& t : net.transitions())
    {
        if (net.arcsIn(t).empty())
        {
            std::cout << (file ? argv[1] : "default net") << ": " << t.key
                      << " has no input place: it waits for inject(), not benchmarked" << std::endl;
            return EXIT_SUCCESS;
        }
    }

    Runtime::Options options;
    options.log = true; // Symmetri keeps its journal too
    Runtime runtime(net, std::make_shared<InlineExecutor>(), options);
    if (!file)
    {
        runtime.on("t0", Simple{});
        runtime.on("t1", Simple{});
    }

    using Clock = std::chrono::steady_clock;
    struct Run
    {
        Color result;
        double us;
        size_t firings;
    };
    auto once = [&]() {
        auto instance = runtime.instantiate("benchmark", {}, goal);
        Clock::time_point const begin = Clock::now();
        Color const result = runtime.run(*instance);
        Clock::time_point const end = Clock::now();
        size_t firings = 0u;
        for (auto const& entry : instance->log())
            firings += (entry.event == LogEntry::Event::Scheduled) ? 1u : 0u;
        return Run{ result, std::chrono::duration<double, std::micro>(end - begin).count(), firings };
    };

    g_firings = 0u;
    Run const first = once();
    std::cout << (file ? argv[1] : "default net") << ": " << net.colors.name(first.result) << " after "
              << first.firings << " firings (" << g_firings << " functions called) in " << first.us
              << " us (first run)";
    if (first.result == Failed)
        std::cout << ": a cycle of transitions without duration, stopped by max_firings_per_date";
    std::cout << std::endl;

    double total = 0.0;
    for (size_t i = 0u; i < runs; ++i)
        total += once().us;
    double const mean = total / double(runs);
    std::cout << "mean of " << runs << " runs: " << mean << " us per run, "
              << (double(first.firings) / mean) << " million firings per second" << std::endl;
    return EXIT_SUCCESS;
}
