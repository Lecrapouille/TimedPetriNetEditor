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
#  include "TimedPetriNetEditor/SparseMatrix.hpp"
#  include "TimedPetriNetEditor/TropicalAlgebra.hpp"
#undef protected
#undef private

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestEventGraph, TestHoward2)
{
    std::string error;
    std::vector<Arc*> erroneous_arcs;

    Net net(TypeOfNet::TimedPetriNet);
    Net canonic(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net,"../data/examples/Howard2.json").c_str(), "");
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(isEventGraph(net), true);
    ASSERT_EQ(erroneous_arcs.empty(), true);

    toCanonicalForm(net, canonic);
    ASSERT_EQ(canonic.isEmpty(), false);
    ASSERT_EQ(isEventGraph(canonic, error, erroneous_arcs), true);
    ASSERT_STREQ(error.c_str(), "");
    ASSERT_EQ(erroneous_arcs.empty(), true);
    ASSERT_STREQ(saveToFile(canonic,"/tmp/canonic.json").c_str(), "");
    ASSERT_EQ(canonic.m_next_place_id, 6u);
    ASSERT_EQ(canonic.m_next_transition_id, 5u);
    ASSERT_EQ(canonic.m_places.size(), 6u);
    ASSERT_EQ(canonic.m_transitions.size(), 5u);
    ASSERT_EQ(canonic.m_arcs.size(), 12u);

    ASSERT_EQ(canonic.m_places[0].tokens, 1u);
    ASSERT_EQ(canonic.m_places[1].tokens, 0u);
    ASSERT_EQ(canonic.m_places[2].tokens, 0u);
    ASSERT_EQ(canonic.m_places[3].tokens, 0u);
    ASSERT_EQ(canonic.m_places[4].tokens, 0u);
    ASSERT_EQ(canonic.m_places[5].tokens, 1u);
}

//------------------------------------------------------------------------------
// https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html
// Chapter 5.2 A Comparison Between Counter and Dater Descriptions
TEST(TestEventGraph, TestToCounterEquation)
{
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net, "../data/examples/EventGraph.json").c_str(), "");
    net.generateArcsInArcsOut(); // FIXME
    ASSERT_EQ(isEventGraph(net), true);

    std::stringstream expected, obtained;

    // --
    expected.str(
        "# Timed event graph represented as counter equation:\n"
        "# T1(t) = min(T0(t - 1), 1 + T2(t - 1), 2 + T1(t - 1))\n"
        "# T2(t) = min(1 + T1(t - 1), T0(t - 2))\n"
        "# T3(t) = min(T1(t), T2(t))\n");
    obtained = showCounterEquation(net, "# ", false, false);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as counter equation:\n"
        "# x1(t) = min(u(t - 1), 1 + x2(t - 1), 2 + x1(t - 1))\n"
        "# x2(t) = min(1 + x1(t - 1), u(t - 2))\n"
        "# y(t) = min(x1(t), x2(t))\n");
    obtained = showCounterEquation(net, "# ", true, false);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as counter equation (min-plus algebra):\n"
        "# T1(t) = T0(t - 1) (+) 1 T2(t - 1) (+) 2 T1(t - 1)\n"
        "# T2(t) = 1 T1(t - 1) (+) T0(t - 2)\n"
        "# T3(t) = T1(t) (+) T2(t)\n");
    obtained = showCounterEquation(net, "# ", false, true);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as counter equation (min-plus algebra):\n"
        "# x1(t) = u(t - 1) (+) 1 x2(t - 1) (+) 2 x1(t - 1)\n"
        "# x2(t) = 1 x1(t - 1) (+) u(t - 2)\n"
        "# y(t) = x1(t) (+) x2(t)\n");
    obtained = showCounterEquation(net, "# ", true, true);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());
}

//------------------------------------------------------------------------------
// QQ: Ce test est bon les matrices sont affichees dans le bon ordre (comme dans JPQ et affichees comme Julia)
TEST(TestEventGraph, TestToAdjacencyMatrices)
{
    std::vector<Arc*> erroneous_arcs; std::string error;
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net, "../data/examples/Howard2.json").c_str(), "");
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), true);
    ASSERT_EQ(error.empty(), true);
    ASSERT_EQ(erroneous_arcs.empty(), true);

    SparseMatrix<MaxPlus> tokens;
    SparseMatrix<MaxPlus> durations;
    ASSERT_EQ(toAdjacencyMatrices(net, tokens, durations), true);
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = true;
    std::cout << "tokens:\n" << tokens << "durations:\n" << durations << std::endl;

    //
    //     | .  .  2  . |       | .  .  5  . |
    //     | 0  .  .  . |       | 5  .  .  . |
    // T = | .  0  .  0 |,  N = | .  3  .  1 |
    //     | 0  .  .  . |       | 1  .  .  . |

    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    std::stringstream ss; ss << tokens;
    ASSERT_STREQ(ss.str().c_str(), "[1, 2, 3, 4, 3], [3, 1, 2, 1, 4], MP([2, 0, 0, 0, 0]), 4, 4");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ss.str(""); ss << tokens;
    ASSERT_STREQ(ss.str().c_str(), ". . 2 . \n0 . . . \n. 0 . 0 \n0 . . . \n");
    ASSERT_EQ(tokens.i.size(), 5u);
    ASSERT_EQ(tokens.j.size(), 5u);
    ASSERT_EQ(tokens.d.size(), 5u);
    ASSERT_EQ(tokens.N, 4u);
    ASSERT_EQ(tokens.M, 4u);
    ASSERT_THAT(tokens.i, ElementsAre(1, 2, 3, 4, 3));
    ASSERT_THAT(tokens.j, ElementsAre(3, 1, 2, 1, 4));
    ASSERT_THAT(tokens.d, ElementsAre(2, 0, 0, 0, 0));

    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    ss.str(""); ss << durations;
    ASSERT_STREQ(ss.str().c_str(), "[1, 2, 3, 4, 3], [3, 1, 2, 1, 4], MP([5, 5, 3, 1, 1]), 4, 4");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ss.str(""); ss << durations;
    ASSERT_STREQ(ss.str().c_str(), ". . 5 . \n5 . . . \n. 3 . 1 \n1 . . . \n");
    ASSERT_EQ(durations.i.size(), 5u);
    ASSERT_EQ(durations.j.size(), 5u);
    ASSERT_EQ(durations.d.size(), 5u);
    ASSERT_EQ(durations.N, 4u);
    ASSERT_EQ(durations.M, 4u);
    ASSERT_THAT(durations.i, ElementsAre(1, 2, 3, 4, 3));
    ASSERT_THAT(durations.j, ElementsAre(3, 1, 2, 1, 4));
    ASSERT_THAT(durations.d, ElementsAre(5, 5, 3, 1, 1));
}

//------------------------------------------------------------------------------
// https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html
// Chapter 1.1. Preliminary Remarks and Some Notation
TEST(TestEventGraph, TestToSysLinInputOutput)
{
    std::vector<Arc*> erroneous_arcs;
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net, "../data/examples/JPQ.json").c_str(), "");
    ASSERT_EQ(isEventGraph(net), true);

    SparseMatrix<MaxPlus> D;
    SparseMatrix<MaxPlus> A;
    SparseMatrix<MaxPlus> B;
    SparseMatrix<MaxPlus> C;

    ASSERT_EQ(toSysLin(net, D, A, B, C), true);
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = true;
    std::cout << "D:\n" << D << "A:\n" << A << "B:\n" << B << "C:\n" << C << std::endl;

    //     | .  . |      | 3  7 |      | . |
    // D = | .  . |, A = | 2  4 |, B = | 1 |, C = | 3 . |
    ASSERT_EQ(D.i.size(), 0u);
    ASSERT_EQ(D.j.size(), 0u);
    ASSERT_EQ(D.d.size(), 0u);
    ASSERT_EQ(D.N, 2u);
    ASSERT_EQ(D.M, 2u);

    ASSERT_EQ(A.i.size(), 4u);
    ASSERT_EQ(A.j.size(), 4u);
    ASSERT_EQ(A.d.size(), 4u);
    ASSERT_EQ(A.N, 2u);
    ASSERT_EQ(A.M, 2u);
    ASSERT_THAT(A.i, ElementsAre(2u, 1u, 1u, 2u));
    ASSERT_THAT(A.j, ElementsAre(1u, 2u, 1u, 2u));
    ASSERT_THAT(A.d, ElementsAre(2.0, 7.0, 3.0, 4.0));

    ASSERT_EQ(B.i.size(), 1u);
    ASSERT_EQ(B.j.size(), 1u);
    ASSERT_EQ(B.d.size(), 1u);
    ASSERT_EQ(B.N, 1u);
    ASSERT_EQ(B.M, 2u);

    ASSERT_THAT(B.i, ElementsAre(2u));
    ASSERT_THAT(B.j, ElementsAre(1u));
    ASSERT_THAT(B.d, ElementsAre(1.0));

    ASSERT_EQ(C.i.size(), 1u);
    ASSERT_EQ(C.j.size(), 1u);
    ASSERT_EQ(C.d.size(), 1u);
    ASSERT_EQ(C.N, 2u);
    ASSERT_EQ(C.M, 1u);
    ASSERT_THAT(C.i, ElementsAre(1u));
    ASSERT_THAT(C.j, ElementsAre(1u));
    ASSERT_THAT(C.d, ElementsAre(3.0));
}

//------------------------------------------------------------------------------
TEST(TestEventGraph, TestToSysLinNoInputNoOutput)
{
    std::vector<Arc*> erroneous_arcs; std::string error;
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net, "../data/examples/Howard2.json").c_str(), "");
    ASSERT_EQ(isEventGraph(net), true);
    ASSERT_EQ(isEventGraph(net, error, erroneous_arcs), true);
    ASSERT_EQ(error.empty(), true);
    ASSERT_EQ(erroneous_arcs.empty(), true);

    SparseMatrix<MaxPlus> D;
    SparseMatrix<MaxPlus> A;
    SparseMatrix<MaxPlus> B;
    SparseMatrix<MaxPlus> C;

    ASSERT_EQ(toSysLin(net, D, A, B, C), true);
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = true;
    std::cout << "D:\n" << D << "A:\n" << A << "B:\n" << B << "C:\n" << C << std::endl;

    //
    //       j
    //   i | .  5  .  1  . |       | .  .  .  .  . |
    //     | .  .  3  .  . |       | .  .  .  .  . |
    // D = | .  .  .  .  . |,  A = | .  .  .  .  5 |
    //     | .  .  1  .  . |       | .  .  .  .  . |
    //     | .  .  .  .  . |       | 0  .  .  .  . |
    //
    ASSERT_EQ(D.i.size(), 4u);
    ASSERT_EQ(D.j.size(), 4u);
    ASSERT_EQ(D.d.size(), 4u);
    ASSERT_EQ(D.N, 5u);
    ASSERT_EQ(D.M, 5u);
    ASSERT_THAT(D.i, ElementsAre(1u, 2u, 1u, 4u));
    ASSERT_THAT(D.j, ElementsAre(4u, 3u, 2u, 3u));
    ASSERT_THAT(D.d, ElementsAre(1.0, 3.0, 5.0, 1.0));
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    std::stringstream ssD; ssD << D;
    ASSERT_STREQ(ssD.str().c_str(), "[1, 2, 1, 4], [4, 3, 2, 3], MP([1, 3, 5, 1]), 5, 5");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ssD.str(""); ssD << D;
    ASSERT_STREQ(ssD.str().c_str(), ". 5 . 1 . \n. . 3 . . \n. . . . . \n. . 1 . . \n. . . . . \n");

    ASSERT_EQ(A.i.size(), 2u);
    ASSERT_EQ(A.j.size(), 2u);
    ASSERT_EQ(A.d.size(), 2u);
    ASSERT_EQ(A.N, 5u);
    ASSERT_EQ(A.M, 5u);
    ASSERT_THAT(A.j, ElementsAre(1u, 5u));
    ASSERT_THAT(A.i, ElementsAre(5u, 3u));
    ASSERT_THAT(A.d, ElementsAre(0.0, 5.0));
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    std::stringstream ssA; ssA << A;
    ASSERT_STREQ(ssA.str().c_str(), "[5, 3], [1, 5], MP([0, 5]), 5, 5");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ssA.str(""); ssA << A;
    // FIXME trailing white space and extra \n not proper
    ASSERT_STREQ(ssA.str().c_str(), ". . . . . \n. . . . . \n. . . . 5 \n. . . . . \n0 . . . . \n");

    ASSERT_EQ(B.i.size(), 0u);
    ASSERT_EQ(B.j.size(), 0u);
    ASSERT_EQ(B.d.size(), 0u);
    ASSERT_EQ(B.N, 0u);
    ASSERT_EQ(B.M, 5u);
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    std::stringstream ssB; ssB << B;
    ASSERT_STREQ(ssB.str().c_str(), "[], [], MP([]), 5, 0");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ssB.str(""); ssB << B;
    ASSERT_STREQ(ssB.str().c_str(), "");

    ASSERT_EQ(C.i.size(), 0u);
    ASSERT_EQ(C.j.size(), 0u);
    ASSERT_EQ(C.d.size(), 0u);
    ASSERT_EQ(C.N, 5u);
    ASSERT_EQ(C.M, 0u);
    SparseMatrix<MaxPlus>::display_for_julia = true;
    SparseMatrix<MaxPlus>::display_as_dense = false;
    std::stringstream ssC; ssC << C;
    ASSERT_STREQ(ssC.str().c_str(), "[], [], MP([]), 0, 5");
    SparseMatrix<MaxPlus>::display_as_dense = true;
    ssC.str(""); ssC << C;
    ASSERT_STREQ(ssC.str().c_str(), "");
}

//------------------------------------------------------------------------------
// https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html
// Chapter 5.2 A Comparison Between Counter and Dater Descriptions
TEST(TestEventGraph, TestToDaterEquation)
{
    Net net(TypeOfNet::TimedPetriNet);

    ASSERT_STREQ(loadFromFile(net, "../data/examples/EventGraph.json").c_str(), "");
    net.generateArcsInArcsOut(); // FIXME
    ASSERT_EQ(isEventGraph(net), true);

    std::stringstream expected, obtained;

    // --
    expected.str(
        "# Timed event graph represented as dater equation:\n"
        "# T1(n) = max(1 + T0(n), 1 + T2(n - 1), 1 + T1(n - 2))\n"
        "# T2(n) = max(1 + T1(n - 1), 2 + T0(n))\n"
        "# T3(n) = max(T1(n), T2(n))\n");
    obtained = showDaterEquation(net, "# ", false, false);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as dater equation:\n"
        "# x1(n) = max(1 + u(n), 1 + x2(n - 1), 1 + x1(n - 2))\n"
        "# x2(n) = max(1 + x1(n - 1), 2 + u(n))\n"
        "# y(n) = max(x1(n), x2(n))\n");
    obtained = showDaterEquation(net, "# ", true, false);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as dater equation (max-plus algebra):\n"
        "# T1(n) = 1 T0(n) (+) 1 T2(n - 1) (+) 1 T1(n - 2)\n"
        "# T2(n) = 1 T1(n - 1) (+) 2 T0(n)\n"
        "# T3(n) = T1(n) (+) T2(n)\n");
    obtained = showDaterEquation(net, "# ", false, true);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());

    // --
    expected.str(
        "# Timed event graph represented as dater equation (max-plus algebra):\n"
        "# x1(n) = 1 u(n) (+) 1 x2(n - 1) (+) 1 x1(n - 2)\n"
        "# x2(n) = 1 x1(n - 1) (+) 2 u(n)\n"
        "# y(n) = x1(n) (+) x2(n)\n");
    obtained = showDaterEquation(net, "# ", true, true);
    ASSERT_STREQ(obtained.str().c_str(), expected.str().c_str());
}