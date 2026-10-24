// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Colored tokens: the result of a function chooses the branch. Inspect
// returns the color Good or Bad (colors of the net, see its "colors"); the
// token it puts in Inspected has this color, and the arcs Inspected -> Ship
// (Good) and Inspected -> Scrap (Bad) take only the tokens of their color.
//   ./Colors [Colors.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdlib>
#include <iostream>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Extended-Petri-Nets/Colors/Colors.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    Color const good = *net.colors.find("Good");
    Color const bad = *net.colors.find("Bad");

    // The functions run in the thread of run() (InlineExecutor).
    Runtime runtime(net);
    int part = 0;
    runtime.on("Inspect", [&part, good, bad]() {
        bool const ok = (++part != 2); // the second part has a scratch
        std::cout << "inspect part " << part << ": " << (ok ? "Good" : "Bad") << std::endl;
        return ok ? good : bad;
    });
    runtime.on("Ship", []() { std::cout << "  ship it" << std::endl; });
    runtime.on("Scrap", []() { std::cout << "  scrap it" << std::endl; });

    // No goal: the instance ends when nothing can happen any more.
    auto lot = runtime.instantiate("lot_1");
    Color const result = runtime.run(*lot);
    std::cout << "lot_1: " << net.colors.name(result)
              << ", shipped " << lot->marking().count(2u)
              << ", scrapped " << lot->marking().count(3u) << std::endl;
    return (result == Deadlocked) ? EXIT_SUCCESS : EXIT_FAILURE;
}
