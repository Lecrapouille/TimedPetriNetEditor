// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: the example "performance" of Symmetri, timed as
// Benchmark.cpp times the runtime: the net is built before the clock starts,
// fire() alone is timed, once, then the mean of many runs. Built against
// Symmetri, not by the Makefile of this folder (see README.md).
//
//   ./SymmetriBenchmark [net.pnml] [runs]
//
// Beware: Symmetri fires a cycle of transitions without function for ever
// (nets/n1.pnml, nets/sat.pnml): its journal grows until the memory ends.
//=============================================================================

#include <symmetri/symmetri.h>
#include <symmetri/utilities.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
struct Simple {};
size_t g_firings = 0;
bool isSynchronous(const Simple &) { return true; }
symmetri::Token fire(const Simple &) { g_firings++; return symmetri::Success; }

int main(int argc, char *argv[]) {
  using namespace symmetri;
  auto pool = std::make_shared<TaskSystem>(1);
  auto make = [&]() {
    if (argc > 1) return PetriNet({std::string(argv[1])}, "benchmark", pool);
    Net net = {{"t0", {{{"Pa", Success}, {"Pb", Success}}, {{"Pc", Success}}}},
               {"t1", {{{"Pc", Success}, {"Pc", Success}}, {{"Pb", Success}, {"Pb", Success}}}}};
    Marking initial = {{"Pa", Success}, {"Pa", Success}, {"Pa", Success},
                       {"Pa", Success}, {"Pb", Success}, {"Pb", Success}};
    Marking goal = {{"Pb", Success}, {"Pb", Success}, {"Pc", Success}, {"Pc", Success}};
    PetriNet petri(net, "benchmark", pool, initial, goal);
    petri.registerCallbackInPlace<Simple>("t0");
    petri.registerCallback("t1", Simple{});
    return petri;
  };
  size_t const runs = (argc > 2) ? std::strtoul(argv[2], nullptr, 10) : 10000u;
  using C = std::chrono::steady_clock;
  auto once = [&](size_t& firings) {
    auto petri = make();
    auto const b = C::now();
    auto const r = symmetri::fire(petri);
    auto const e = C::now();
    firings = getLog(petri).size() / 2u;
    (void)r;
    return std::chrono::duration<double, std::micro>(e - b).count();
  };
  size_t firings = 0;
  double const first = once(firings);
  std::cout << (argc > 1 ? argv[1] : "default net") << ": " << firings << " firings in " << first << " us (first run)" << std::endl;
  double total = 0.0;
  size_t f = 0;
  for (size_t i = 0; i < runs; ++i) total += once(f);
  double const mean = total / double(runs);
  std::cout << "mean of " << runs << " runs: " << mean << " us per run, " << double(firings) / mean
            << " million firings per second" << std::endl;
}
