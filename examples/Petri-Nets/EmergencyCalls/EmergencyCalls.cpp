// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The emergency call center run by the runtime. When a level 1 operator
// answers, its function returns the kind of the call as a color (Advice,
// Urgent or Critical, colors of the net): the arcs from Call screening take
// the token of their color, so the kind of the call chooses its way. The net
// is a Petri net, not a timed one: the runtime does not wait the durations
// written on its arcs, the journal gives the order of the events.
//   ./EmergencyCalls [EmergencyCalls.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Petri-Nets/EmergencyCalls/EmergencyCalls.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // The kinds of the 7 calls waiting, in their order.
    std::vector<std::string> const calls = { "Critical", "Advice", "Urgent", "Urgent",
                                             "Advice", "Critical", "Urgent" };
    size_t call = 0u;
    Runtime runtime(net);
    runtime.on("Level 1 answers", [&net, &calls, &call]() {
        return *net.colors.find(calls[call++]);
    });

    // No goal: the instance ends when all the calls are handled.
    auto center = runtime.instantiate("monday");
    runtime.run(*center);

    // The journal: the transitions in the order they ended, and the kind of
    // the calls.
    int order = 0;
    for (auto const& entry : center->log())
    {
        if (entry.event != LogEntry::Event::Completed)
            continue;
        std::printf("%3d  %-20s", ++order, entry.transition.c_str());
        if (entry.transition == "Level 1 answers")
            std::printf(" -> %s", net.colors.name(entry.result).c_str());
        std::printf("\n");
    }
    std::printf("%s: %zu urgent and %zu critical calls handled by level 2\n",
                net.colors.name(center->result()).c_str(),
                center->marking().count(11u), center->marking().count(12u));
    return EXIT_SUCCESS;
}
