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
#  include "src/utils/Animation.hpp"
#undef protected
#undef private

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestAnimatedTokenCreation)
{
    // Reminder: AnimatedToken not made for Place -> Transition
    Transition t1(42u, "", 3.5f, 4.0f, 45u, true);
    Place p1(43u, "", 4.6f, 5.1f, 13u);
    Arc a1(t1, p1, 10.0f);
    AnimatedToken at1(a1, 3u, PetriNet::Type::TimedPetri);
    float norm = sqrtf((3.5f - 4.6f) * (3.5f - 4.6f) + (4.0f - 5.1f) * (4.0f - 5.1f));

    ASSERT_EQ(at1.x, 3.5f);
    ASSERT_EQ(at1.y, 4.0f);
    ASSERT_EQ(at1.tokens, 3u);
    ASSERT_EQ(at1.arc.from.type, Node::Transition);
    ASSERT_EQ(at1.arc.from.id, 42u);
    ASSERT_STREQ(at1.arc.from.key.c_str(), "T42");
    ASSERT_EQ(at1.arc.to.type, Node::Place);
    ASSERT_EQ(at1.arc.to.id, 43u);
    ASSERT_STREQ(at1.arc.to.key.c_str(), "P43");
    ASSERT_EQ(at1.magnitude, norm);
    ASSERT_EQ(at1.speed, norm / 10.0f);
    ASSERT_EQ(at1.offset, 0.0f);
    ASSERT_EQ(&at1.toPlace(), &p1);

    AnimatedToken at2(at1);
    ASSERT_EQ(at2.x, 3.5f);
    ASSERT_EQ(at2.y, 4.0f);
    ASSERT_EQ(at2.tokens, 3u);
    ASSERT_EQ(at2.arc.from.type, Node::Transition);
    ASSERT_EQ(at2.arc.from.id, 42u);
    ASSERT_STREQ(at2.arc.from.key.c_str(), "T42");
    ASSERT_EQ(at2.arc.to.type, Node::Place);
    ASSERT_EQ(at2.arc.to.id, 43u);
    ASSERT_STREQ(at2.arc.to.key.c_str(), "P43");
    ASSERT_EQ(at2.magnitude, norm);
    ASSERT_EQ(at2.speed, norm / 10.0f);
    ASSERT_EQ(at2.offset, 0.0f);
    ASSERT_EQ(&at2.toPlace(), &p1);

    Transition t2(45u, "", 13.5f, 14.0f, 145u, true);
    Place p2(46u, "", 14.6f, 15.1f, 113u);
    Arc a2(t2, p2, 110.0f);
    AnimatedToken at3(a2, 13u, PetriNet::Type::TimedPetri);
    ASSERT_EQ(at3.x, 13.5f);
    ASSERT_EQ(at3.y, 14.0f);
    ASSERT_EQ(at3.tokens, 13u);
    ASSERT_EQ(at3.arc.from.type, Node::Transition);
    ASSERT_EQ(at3.arc.from.id, 45u);
    ASSERT_STREQ(at3.arc.from.key.c_str(), "T45");
    ASSERT_EQ(at3.arc.to.type, Node::Place);
    ASSERT_EQ(at3.arc.to.id, 46u);
    ASSERT_STREQ(at3.arc.to.key.c_str(), "P46");
    ASSERT_EQ(&at3.toPlace(), &p2);

    at3 = at1;
    ASSERT_EQ(at3.x, 3.5f);
    ASSERT_EQ(at3.y, 4.0f);
    ASSERT_EQ(at3.tokens, 3u);
    ASSERT_EQ(at3.arc.from.type, Node::Transition);
    ASSERT_EQ(at3.arc.from.id, 42u);
    ASSERT_STREQ(at3.arc.from.key.c_str(), "T42");
    ASSERT_EQ(at3.arc.to.type, Node::Place);
    ASSERT_EQ(at3.arc.to.id, 43u);
    ASSERT_STREQ(at3.arc.to.key.c_str(), "P43");
    ASSERT_EQ(at3.magnitude, norm);
    ASSERT_EQ(at3.speed, norm / 10.0f);
    ASSERT_EQ(at3.offset, 0.0f);
    ASSERT_EQ(&at3.toPlace(), &p1);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestAnimatedTokenUpdate)
{
    // T1 --> P1 is 20 unit of distance along the X-axis
    Transition t1(42u, "", 0.0f, 0.0f, 45u, true);
    Place p1(43u, "", 20.0f, 0.0f, 13u);
    Arc a1(t1, p1, 10.0f); // Duration: 10 units of time
    AnimatedToken at1(a1, 3u, PetriNet::Type::TimedPetri);
    ASSERT_EQ(at1.magnitude, 20.0f);
    ASSERT_EQ(at1.speed, 2.0f); // 20 units of distance / 10 units of time

    // 1st unit of time
    ASSERT_EQ(at1.update(1.0f), false);
    ASSERT_EQ(at1.offset, 0.1f);
    ASSERT_EQ(at1.x, 2.0f);
    ASSERT_EQ(at1.y, 0.0f);

    // 2nd unit of time
    ASSERT_EQ(at1.update(1.0f), false);
    ASSERT_EQ(at1.offset, 0.2f);
    ASSERT_EQ(at1.x, 4.0f);
    ASSERT_EQ(at1.y, 0.0f);

    // 9th unit of time
    ASSERT_EQ(at1.update(7.0f), false);
    ASSERT_EQ(at1.offset, 0.9f);
    ASSERT_EQ(at1.x, 18.0f);
    ASSERT_EQ(at1.y, 0.0f);

    // 10th unit of time
    ASSERT_EQ(at1.update(1.0f), true);
    ASSERT_EQ(at1.offset, 1.0f);
    ASSERT_EQ(at1.x, 20.0f);
    ASSERT_EQ(at1.y, 0.0f);
}
