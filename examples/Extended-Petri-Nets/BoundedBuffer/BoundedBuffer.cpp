// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The bounded buffer run by the runtime, on a simulated clock: the producer
// makes a part every time unit, the packer packs 2 parts every 3 time units.
// The runtime follows the extensions of the net: the capacity of Buffer (4)
// blocks the producer, the inhibitor arc from Stopped stops it, the reset arc
// of Flush empties Buffer. Stop and Ask flush have no input place: the
// program injects them, as a button or a message would. Restart has one
// (Stopped): a guard holds it until the operator asks.
//   ./BoundedBuffer [BoundedBuffer.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // The rhythm of the machines: guards on the date of the instance.
    double now = 0.0;
    double next_part = 0.0;
    double next_pack = 3.0;
    bool restart = false;
    Runtime runtime(net);
    runtime.on("Produce", [&now, &next_part]() { next_part = now + 1.0; });
    runtime.guard("Produce", [&next_part](Context const& c) { return c.now >= next_part; });
    runtime.on("Pack", [&now, &next_pack]() { next_pack = now + 3.0; });
    runtime.guard("Pack", [&next_pack](Context const& c) { return c.now >= next_pack; });
    runtime.on("Restart", [&restart]() { restart = false; });
    runtime.guard("Restart", [&restart](Context const&) { return restart; });

    auto line = runtime.instantiate("line");
    std::printf(" date  Buffer  Packed  Stopped  event\n");
    for (int date = 0; date <= 20; ++date)
    {
        char const* event = "";
        if (date == 8) { runtime.inject(*line, "Stop"); event = "Stop: the producer waits"; }
        if (date == 12) { restart = true; event = "Restart"; }
        if (date == 15) { runtime.inject(*line, "Ask flush"); event = "Ask flush: Buffer emptied"; }
        now = double(date);
        if (!runtime.step(*line, now))
            break;
        ColoredMarking const& m = line->marking();
        std::printf("%5d  %6zu  %6zu  %7zu  %s\n", date, m.count(1u), m.count(2u), m.count(3u), event);
    }
    runtime.cancel(*line);
    runtime.run(*line);
    return EXIT_SUCCESS;
}
