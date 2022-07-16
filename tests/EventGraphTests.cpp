//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#include "main.hpp"
#define protected public
#define private public
#  include "src/PetriNet.hpp"
#  include "src/utils/Utils.hpp"
#undef protected
#undef private

//------------------------------------------------------------------------------
TEST(TestEventGraph, TestHoward2)
{
    PetriNet net(PetriNet::Behavior::TimedPetri);
    PetriNet canonic(PetriNet::Behavior::TimedPetri);

    ASSERT_EQ(net.load("../examples/Howard2.json"), true);
    ASSERT_EQ(net.isEmpty(), false);
    ASSERT_EQ(net.isEventGraph(), true);

    net.toCanonicalForm(canonic); // FIXME shall return bool isEventGraph() ?
    canonic.generateArcsInArcsOut(); // FIXME

    ASSERT_EQ(canonic.isEmpty(), false);
    ASSERT_EQ(canonic.isEventGraph(), true);
    ASSERT_EQ(canonic.save("/tmp/canonic.json"), true);
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
TEST(TestEventGraph, TesSparseMatrixConstructor)
{
    SparseMatrix M;

    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 0u);
    ASSERT_EQ(M.M, 0u);

    M.dim(4u, 5u);
    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);

    M.add(0u, 0u, 42.0);
    ASSERT_EQ(M.i.size(), 1u);
    ASSERT_EQ(M.j.size(), 1u);
    ASSERT_EQ(M.d.size(), 1u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);

    M.add(4u, 5u, 43.0);
    ASSERT_EQ(M.i.size(), 2u);
    ASSERT_EQ(M.j.size(), 2u);
    ASSERT_EQ(M.d.size(), 2u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);
    ASSERT_EQ(M.i[1], 5u);
    ASSERT_EQ(M.j[1], 6u);
    ASSERT_EQ(M.d[1], 43.0);

    // Check double insertion is possible (no security check)
    M.add(4u, 5u, 44.0);
    ASSERT_EQ(M.i.size(), 3u);
    ASSERT_EQ(M.j.size(), 3u);
    ASSERT_EQ(M.d.size(), 3u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);
    ASSERT_EQ(M.i[1], 5u);
    ASSERT_EQ(M.j[1], 6u);
    ASSERT_EQ(M.d[1], 43.0);
    ASSERT_EQ(M.i[2], 5u);
    ASSERT_EQ(M.j[2], 6u);
    ASSERT_EQ(M.d[2], 44.0);

    M.clear();
    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
}

//------------------------------------------------------------------------------
TEST(TestEventGraph, TestToSysLin)
{
    PetriNet net(PetriNet::Behavior::TimedPetri);

    ASSERT_EQ(net.load("../examples/Howard2.json"), true); // FIXME shall call generateArcsInArcsOut ?
    net.generateArcsInArcsOut(); // FIXME

    ASSERT_EQ(net.isEventGraph(), true);

    SparseMatrix D;
    SparseMatrix A;
    SparseMatrix B;
    SparseMatrix C;

    ASSERT_EQ(net.toSysLin(D, A, B, C), true);
    //
    //       j
    //   i | .  .  .  .  . |       | .  .  .  .  0 |
    //     | 5  .  .  .  . |       | .  .  .  .  . |
    // D = | .  3  .  1  . |,  A = | .  .  .  .  . |
    //     | 1  .  .  .  . |       | .  .  .  .  . |
    //     | .  .  .  .  . |       | .  .  5  .  . |
    //
    ASSERT_EQ(D.i.size(), 4u);
    ASSERT_EQ(D.j.size(), 4u);
    ASSERT_EQ(D.d.size(), 4u);
    ASSERT_EQ(D.N, 5u);
    ASSERT_EQ(D.M, 5u);
    ASSERT_THAT(D.i, UnorderedElementsAre(2u, 3u, 3u, 4u));
    ASSERT_THAT(D.j, UnorderedElementsAre(1u, 1u, 2u, 4u));
    ASSERT_THAT(D.d, UnorderedElementsAre(5.0, 1.0, 3.0, 1.0));

    ASSERT_EQ(A.i.size(), 2u);
    ASSERT_EQ(A.j.size(), 2u);
    ASSERT_EQ(A.d.size(), 2u);
    ASSERT_EQ(A.N, 5u);
    ASSERT_EQ(A.M, 5u);
    ASSERT_THAT(A.i, UnorderedElementsAre(1u, 5u));
    ASSERT_THAT(A.j, UnorderedElementsAre(3u, 5u));
    ASSERT_THAT(A.d, UnorderedElementsAre(0.0, 5.0));

    ASSERT_EQ(B.i.size(), 0u);
    ASSERT_EQ(B.j.size(), 0u);
    ASSERT_EQ(B.d.size(), 0u);
    ASSERT_EQ(B.N, 0u);
    ASSERT_EQ(B.M, 0u);

    ASSERT_EQ(C.i.size(), 0u);
    ASSERT_EQ(C.j.size(), 0u);
    ASSERT_EQ(C.d.size(), 0u);
    ASSERT_EQ(C.N, 0u);
    ASSERT_EQ(C.M, 0u);
}

