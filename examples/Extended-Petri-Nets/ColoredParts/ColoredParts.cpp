// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Colored parts: two products share two machines. The colors P1 and P2 of
// the net have their own RGB value (red and blue, "colors" of the JSON
// file): the editor draws the parts with it. Each transition takes and gives
// the parts of one product, so a part keeps its color from the stock to the
// end; the runtime prints each operation and the parts done by product.
//   ./ColoredParts [ColoredParts.json]

#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdlib>
#include <iostream>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Extended-Petri-Nets/ColoredParts/ColoredParts.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    Color const p1 = *net.colors.find("P1");
    Color const p2 = *net.colors.find("P2");
    std::cout << "P1 drawn " << toHex(*net.colors.rgb(p1)) << ", P2 drawn "
              << toHex(*net.colors.rgb(p2)) << std::endl;

    // One function per operation; they run in the thread of run().
    Runtime runtime(net);
    for (Transition const& transition : net.transitions())
    {
        std::string const name = transition.caption;
        runtime.on(name, [name]() { std::cout << "  " << name << std::endl; });
    }

    // No goal: the instance ends when nothing can happen any more.
    auto lot = runtime.instantiate("lot_1");
    Color const result = runtime.run(*lot);
    size_t const done = 4u; // the place Done
    std::cout << "lot_1: " << net.colors.name(result) << ", done "
              << lot->marking().count(done, p1) << " P1 and "
              << lot->marking().count(done, p2) << " P2" << std::endl;
    return (lot->marking().count(done, p1) == 2u) && (lot->marking().count(done, p2) == 2u)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
