// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The landing gear net as the safety layer of a flight: the pilot gives
// orders (a guard holds each transition until its order comes), the net only
// lets the allowed ones fire. Start warning has no guard: it fires by itself
// as soon as the plane flies fast with its gear down.
//   ./LandingGear [LandingGear.json]

#include "IO/IO.hpp"
#include "PetriNet/Firing.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace tpne;

int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Petri-Nets/LandingGear/LandingGear.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }

    // The orders of the pilot not done yet, by transition.
    std::map<std::string, bool> order;
    Runtime runtime(net);
    for (char const* name : { "Faster", "Slower", "Retract", "Extend", "Ack" })
    {
        runtime.guard(name, [&order, name](Context const&) { return order[name]; });
        runtime.on(name, [&order, name]() {
            order[name] = false;
            std::cout << "        " << name << " done" << std::endl;
        });
    }
    runtime.on("Start warning", []() { std::cout << "        WARNING: gear down at high speed!" << std::endl; });

    struct Order { double date; char const* transition; };
    std::vector<Order> const flight = {
        { 1.0, "Faster" },   // take off, the gear still down
        { 2.0, "Retract" },
        { 3.0, "Ack" },
        { 4.0, "Extend" },   // too early: still at high speed
        { 5.0, "Slower" },   // then the gear can go down
    };

    auto plane = runtime.instantiate("flight");
    runtime.step(*plane, 0.0);
    for (auto const& o : flight)
    {
        size_t const t = runtime.transitionOf(o.transition);
        bool const allowed = isEnabled(net, plane->marking().counts(), t);
        std::cout << "t = " << o.date << "  pilot: " << o.transition
                  << (allowed ? "" : "  (refused by the net, the order waits)") << std::endl;
        order[o.transition] = true;
        runtime.step(*plane, o.date);
    }
    runtime.cancel(*plane);
    runtime.run(*plane);

    std::cout << "end:";
    for (auto const& p : net.places())
        if (plane->marking().count(p.id) > 0u)
            std::cout << " [" << p.caption << "]";
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
