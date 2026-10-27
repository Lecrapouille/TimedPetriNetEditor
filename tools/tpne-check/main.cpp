// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// tpne-check: print the properties of nets and fail when one can deadlock,
// for a CI:
//   tpne-check [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json ...
// Exit code: 0 no deadlock, 1 a net can deadlock, 2 bad usage or file.

#include "Analysis/Analysis.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace tpne;

//------------------------------------------------------------------------------
static int usage(char const* program)
{
    std::cerr << "Usage: " << program << " [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json ...\n"
              << "Print the properties of the nets: deadlocks, dead transitions, bounds,\n"
              << "liveness, invariants. --dot writes the graph of the markings of the last\n"
              << "net (Graphviz). Exit code: 0 no deadlock, 1 a net can deadlock, 2 bad\n"
              << "usage or file.\n";
    return 2;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    size_t max_states = 100000u;
    bool allow_deadlock = false;
    std::string dot;
    int status = 0;
    int files = 0;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--max-states") == 0)
        {
            if (++i == argc)
                return usage(argv[0]);
            max_states = std::strtoul(argv[i], nullptr, 10);
            if (max_states == 0u)
                return usage(argv[0]);
            continue;
        }
        if (std::strcmp(argv[i], "--dot") == 0)
        {
            if (++i == argc)
                return usage(argv[0]);
            dot = argv[i];
            continue;
        }
        if (std::strcmp(argv[i], "--allow-deadlock") == 0)
        {
            allow_deadlock = true;
            continue;
        }
        if ((std::strcmp(argv[i], "-h") == 0) || (std::strcmp(argv[i], "--help") == 0))
        {
            usage(argv[0]);
            return 0;
        }

        ++files;
        Net net;
        bool springify;
        std::string const error = loadFromFile(net, argv[i], springify);
        if (!error.empty())
        {
            std::cerr << argv[i] << ": " << error << std::endl;
            status = 2;
            continue;
        }

        std::cout << argv[i] << ":\n" << report(net, max_states) << std::endl;
        if (!dot.empty())
        {
            ReachabilityGraph const graph = reachabilityGraph(net, max_states);
            std::ofstream file(dot);
            file << toDot(net, graph.states, graph.edges, graph.colored);
            if (!file)
            {
                std::cerr << "Cannot write " << dot << std::endl;
                status = 2;
            }
        }
        if (!allow_deadlock && (status == 0) && properties(net, max_states).deadlock)
            status = 1;
    }

    return (files == 0) ? usage(argv[0]) : status;
}
