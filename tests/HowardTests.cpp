//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "main.hpp"
#define protected public
#define private public
#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "TimedPetriNetEditor/Algorithms.hpp"
#  include "Net/Howard.h"
#undef protected
#undef private

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestHoward, TestSemiSimple)
{
    std::vector<double> timings = {
        0.0, 1.0, 0.0, 1.0, 2.0
    };
    std::vector<double> delays = {
        1.0, 0.0, 0.0, 0.0, 1.0
    };
    std::vector<int> arcs = {
        0, 1,   1, 0,   2, 0,
        2, 1,   2, 2,
    };

    size_t const nnodes = 3u;
    size_t const narcs = 5u;
    ASSERT_EQ(timings.size(), narcs);
    ASSERT_EQ(delays.size(), narcs);
    ASSERT_EQ(arcs.size(), 2u * narcs);
    ASSERT_EQ(*max_element(std::begin(arcs), std::end(arcs)), int(nnodes - 1u));

    std::vector<double> v(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> pi(nnodes); // optimal policy
    int ncomponents; // Number of connected components of the optimal policy
    int niterations; // Number of iteration needed by the algorithm
    int verbosemode = 0; // No verbose
    int res = Semi_Howard(arcs.data(), timings.data(), delays.data(),
                          int(nnodes), int(narcs),
                          chi.data(), v.data(), pi.data(),
                          &niterations, &ncomponents, verbosemode);

    ASSERT_EQ(res, 0);

    ASSERT_EQ(v.size(), nnodes);
    ASSERT_EQ(chi.size(), nnodes);
    ASSERT_EQ(pi.size(), nnodes);
    ASSERT_EQ(ncomponents, 2);

    ASSERT_DOUBLE_EQ(chi[0], 1.0); ASSERT_DOUBLE_EQ(v[0], 0.0); ASSERT_EQ(pi[0], 1);
    ASSERT_DOUBLE_EQ(chi[1], 1.0); ASSERT_DOUBLE_EQ(v[1], 1.0); ASSERT_EQ(pi[1], 0);
    ASSERT_DOUBLE_EQ(chi[2], 2.0); ASSERT_DOUBLE_EQ(v[2], 2.0); ASSERT_EQ(pi[2], 2);
}

//------------------------------------------------------------------------------
TEST(TestHoward, TestSemiNetherlands)
{
    std::vector<double> timings = {
        61.0, 81.0, 58.0, 0.0, 86.0, 69.0, 69.0, 36.0, 35.0, 0.0, 58.0, 61.0
    };
    std::vector<double> delays = {
        2.0, 1.0, 1.0, 0.0, 2.0, 2.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0
    };
    std::vector<int> arcs = {
        0, 1,   1, 3,   2, 0,
        2, 5,   3, 2,   3, 4,
        4, 3,   4, 6,   5, 4,
        6, 1,   6, 7,   7, 5,
    };

    size_t const nnodes = 8u;
    size_t const narcs = 12u;
    ASSERT_EQ(timings.size(), narcs);
    ASSERT_EQ(delays.size(), narcs);
    ASSERT_EQ(arcs.size(), 2u * narcs);
    ASSERT_EQ(*max_element(std::begin(arcs), std::end(arcs)), int(nnodes - 1u));

    std::vector<double> v(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> pi(nnodes); // optimal policy
    int ncomponents; // Number of connected components of the optimal policy
    int niterations; // Number of iteration needed by the algorithm
    int verbosemode = 0; // No verbose
    int res = Semi_Howard(arcs.data(), timings.data(), delays.data(),
                          int(nnodes), int(narcs),
                          chi.data(), v.data(), pi.data(),
                          &niterations, &ncomponents, verbosemode);

    ASSERT_EQ(res, 0);

    ASSERT_EQ(v.size(), nnodes);
    ASSERT_EQ(chi.size(), nnodes);
    ASSERT_EQ(pi.size(), nnodes);
    ASSERT_EQ(ncomponents, 1);

    ASSERT_DOUBLE_EQ(chi[0], 47.666666666666664); ASSERT_DOUBLE_EQ(v[0], 47.6666666666667); ASSERT_EQ(pi[0], 1);
    ASSERT_DOUBLE_EQ(chi[1], 47.666666666666664); ASSERT_DOUBLE_EQ(v[1], 82.0); ASSERT_EQ(pi[1], 3);
    ASSERT_DOUBLE_EQ(chi[2], 47.666666666666664); ASSERT_DOUBLE_EQ(v[2], 58.000000000000036); ASSERT_EQ(pi[2], 0);
    ASSERT_DOUBLE_EQ(chi[3], 47.666666666666664); ASSERT_DOUBLE_EQ(v[3], 48.666666666666693); ASSERT_EQ(pi[3], 2);
    ASSERT_DOUBLE_EQ(chi[4], 47.666666666666664); ASSERT_DOUBLE_EQ(v[4], 70.333333333333371); ASSERT_EQ(pi[4], 6);
    ASSERT_DOUBLE_EQ(chi[5], 47.666666666666664); ASSERT_DOUBLE_EQ(v[5], 57.666666666666707); ASSERT_EQ(pi[5], 4);
    ASSERT_DOUBLE_EQ(chi[6], 47.666666666666664); ASSERT_DOUBLE_EQ(v[6], 82.000000000000036); ASSERT_EQ(pi[6], 1);
    ASSERT_DOUBLE_EQ(chi[7], 47.666666666666664); ASSERT_DOUBLE_EQ(v[7], 71.0); ASSERT_EQ(pi[7], 5);
}

//------------------------------------------------------------------------------
TEST(TestHoward, TestPetriNetSemiSimple)
{
    Net net(TypeOfNet::TimedPetriNet);

    // Check dummy result is set to "invalid".
    CriticalCycleResult res;
    ASSERT_EQ(res.success, false);
    ASSERT_EQ(res.eigenvector.size(), 0u);
    ASSERT_EQ(res.cycle_time.size(), 0u);
    ASSERT_EQ(res.optimal_policy.size(), 0u);
    ASSERT_EQ(res.arcs.size(), 0u);
    ASSERT_STREQ(res.message.str().c_str(), "");

    // Load a net that is not event graph
    ASSERT_STREQ(loadFromFile(net,"data/AppelsDurgence.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedPetriNet);
    ASSERT_EQ(net.isEmpty(), false);
    res = findCriticalCycle(net);
    ASSERT_EQ(res.success, false);
    ASSERT_EQ(res.eigenvector.size(), 0u);
    ASSERT_EQ(res.cycle_time.size(), 0u);
    ASSERT_EQ(res.optimal_policy.size(), 0u);
    ASSERT_NE(res.arcs.size(), 0u);
    ASSERT_STREQ(res.message.str().c_str(), "The Petri net is not an event graph. Because:\n  P0 has more than one output arc: T0 T4 T8\n");

    // Load a net that is an event graph but that Howard does find policy (FIXME while it should)
    ASSERT_STREQ(loadFromFile(net,"data/EventGraph.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedEventGraph);
    ASSERT_EQ(net.isEmpty(), false);
    res = findCriticalCycle(net);
    ASSERT_EQ(res.success, false);
    ASSERT_EQ(res.eigenvector.size(), 0u);
    ASSERT_EQ(res.cycle_time.size(), 0u);
    ASSERT_EQ(res.optimal_policy.size(), 0u);
    ASSERT_EQ(res.arcs.size(), 0u);
    ASSERT_STREQ(res.message.str().c_str(), "No policy found");

    // Load a net that is an event graph
    ASSERT_STREQ(loadFromFile(net,"data/Howard2.json").c_str(), "");
    ASSERT_EQ(net.type(), TypeOfNet::TimedEventGraph);
    ASSERT_EQ(net.isEmpty(), false);
    res = findCriticalCycle(net);
    ASSERT_EQ(res.success, true);
    ASSERT_EQ(res.eigenvector.size(), 4u);
    ASSERT_EQ(res.eigenvector[0], 0.0f);
    ASSERT_EQ(res.eigenvector[1], 5.0f);
    ASSERT_EQ(res.eigenvector[2], 8.0f);
    ASSERT_EQ(res.eigenvector[3], 1.0f);
    ASSERT_EQ(res.cycle_time.size(), 4u);
    ASSERT_EQ(res.cycle_time[0], 6.5f);
    ASSERT_EQ(res.cycle_time[1], 6.5f);
    ASSERT_EQ(res.cycle_time[2], 6.5f);
    ASSERT_EQ(res.cycle_time[3], 6.5f);
    ASSERT_EQ(res.optimal_policy.size(), 4u);
    ASSERT_EQ(res.optimal_policy[0], 2u);
    ASSERT_EQ(res.optimal_policy[1], 0u);
    ASSERT_EQ(res.arcs.size(), 8u);
    ASSERT_STREQ(res.arcs[0]->from.key.c_str(), "T2");
    ASSERT_STREQ(res.arcs[0]->to.key.c_str(), "P0");
    ASSERT_STREQ(res.arcs[1]->from.key.c_str(), "P0");
    ASSERT_STREQ(res.arcs[1]->to.key.c_str(), "T0");
    ASSERT_STREQ(res.arcs[2]->from.key.c_str(), "T0");
    ASSERT_STREQ(res.arcs[2]->to.key.c_str(), "P1");
    ASSERT_STREQ(res.arcs[3]->from.key.c_str(), "P1");
    ASSERT_STREQ(res.arcs[3]->to.key.c_str(), "T1");
    ASSERT_STREQ(res.arcs[4]->from.key.c_str(), "T1");
    ASSERT_STREQ(res.arcs[4]->to.key.c_str(), "P2");
    ASSERT_STREQ(res.arcs[5]->from.key.c_str(), "P2");
    ASSERT_STREQ(res.arcs[5]->to.key.c_str(), "T2");
    ASSERT_STREQ(res.arcs[6]->from.key.c_str(), "T0");
    ASSERT_STREQ(res.arcs[6]->to.key.c_str(), "P3");
    ASSERT_STREQ(res.arcs[7]->from.key.c_str(), "P3");
    ASSERT_STREQ(res.arcs[7]->to.key.c_str(), "T3");
    ASSERT_STREQ(res.message.str().substr(0u, 14u).c_str(), "Critical cycle");
}
