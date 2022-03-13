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
#undef protected
#undef private

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestHoward2)
{
    PetriNet net;
    PetriNet canonic;

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
