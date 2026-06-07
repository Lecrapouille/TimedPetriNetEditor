// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#define protected public
#define private public
#  include "PetriNet/PetriNet.hpp"
#  include "PetriNet/TimedTokens.hpp"
#undef protected
#undef private

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTimedTokenCreation)
{
    // Reminder: TimedToken not made for Place -> Transition
    Transition t1(42u, "", 3.5f, 4.0f, true);
    Place p1(43u, "", 4.6f, 5.1f, 13u);
    Arc a1(t1, p1, 10.0f);
    TimedToken at1(a1, 3u, TypeOfNet::TimedPetriNet);
    float norm = sqrtf((3.5f - 4.6f) * (3.5f - 4.6f) + (4.0f - 5.1f) * (4.0f - 5.1f));

    ASSERT_EQ(at1.x, 3.5f);
    ASSERT_EQ(at1.y, 4.0f);
    ASSERT_EQ(at1.tokens, 3u);
    ASSERT_EQ(at1.arc->from.type, Node::Type::Transition);
    ASSERT_EQ(at1.arc->from.id, 42u);
    ASSERT_STREQ(at1.arc->from.key.c_str(), "T42");
    ASSERT_EQ(at1.arc->to.type, Node::Type::Place);
    ASSERT_EQ(at1.arc->to.id, 43u);
    ASSERT_STREQ(at1.arc->to.key.c_str(), "P43");
    ASSERT_EQ(at1.magnitude, norm);
    ASSERT_EQ(at1.speed, norm / 10.0f);
    ASSERT_EQ(at1.offset, 0.0f);
    ASSERT_EQ(&at1.toPlace(), &p1);

    TimedToken at2(at1);
    ASSERT_EQ(at2.x, 3.5f);
    ASSERT_EQ(at2.y, 4.0f);
    ASSERT_EQ(at2.tokens, 3u);
    ASSERT_EQ(at2.arc->from.type, Node::Type::Transition);
    ASSERT_EQ(at2.arc->from.id, 42u);
    ASSERT_STREQ(at2.arc->from.key.c_str(), "T42");
    ASSERT_EQ(at2.arc->to.type, Node::Type::Place);
    ASSERT_EQ(at2.arc->to.id, 43u);
    ASSERT_STREQ(at2.arc->to.key.c_str(), "P43");
    ASSERT_EQ(at2.magnitude, norm);
    ASSERT_EQ(at2.speed, norm / 10.0f);
    ASSERT_EQ(at2.offset, 0.0f);
    ASSERT_EQ(&at2.toPlace(), &p1);

    Transition t2(45u, "", 13.5f, 14.0f, true);
    Place p2(46u, "", 14.6f, 15.1f, 113u);
    Arc a2(t2, p2, 110.0f);
    TimedToken at3(a2, 13u, TypeOfNet::TimedPetriNet);
    ASSERT_EQ(at3.x, 13.5f);
    ASSERT_EQ(at3.y, 14.0f);
    ASSERT_EQ(at3.tokens, 13u);
    ASSERT_EQ(at3.arc->from.type, Node::Type::Transition);
    ASSERT_EQ(at3.arc->from.id, 45u);
    ASSERT_STREQ(at3.arc->from.key.c_str(), "T45");
    ASSERT_EQ(at3.arc->to.type, Node::Type::Place);
    ASSERT_EQ(at3.arc->to.id, 46u);
    ASSERT_STREQ(at3.arc->to.key.c_str(), "P46");
    ASSERT_EQ(&at3.toPlace(), &p2);

    at3 = at1;
    ASSERT_EQ(at3.x, 3.5f);
    ASSERT_EQ(at3.y, 4.0f);
    ASSERT_EQ(at3.tokens, 3u);
    ASSERT_EQ(at3.arc->from.type, Node::Type::Transition);
    ASSERT_EQ(at3.arc->from.id, 42u);
    ASSERT_STREQ(at3.arc->from.key.c_str(), "T42");
    ASSERT_EQ(at3.arc->to.type, Node::Type::Place);
    ASSERT_EQ(at3.arc->to.id, 43u);
    ASSERT_STREQ(at3.arc->to.key.c_str(), "P43");
    ASSERT_EQ(at3.magnitude, norm);
    ASSERT_EQ(at3.speed, norm / 10.0f);
    ASSERT_EQ(at3.offset, 0.0f);
    ASSERT_EQ(&at3.toPlace(), &p1);
}

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTimedTokenUpdate)
{
    // T1 --> P1 is 20 unit of distance along the X-axis
    Transition t1(42u, "", 0.0f, 0.0f, true);
    Place p1(43u, "", 20.0f, 0.0f, 13u);
    Arc a1(t1, p1, 10.0f); // Duration: 10 units of time
    TimedToken at1(a1, 3u, TypeOfNet::TimedPetriNet);
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

//------------------------------------------------------------------------------
TEST(TestPetriNet, TestTimedTokenPetriNetVisualDuration)
{
    // Petri net mode: animation duration is fixed (0.2 s), not arc->duration.
    Transition t1(42u, "", 0.0f, 0.0f, true);
    Place p_short(43u, "", 10.0f, 0.0f, 0u);
    Place p_long(44u, "", 200.0f, 0.0f, 0u);
    Arc short_arc(t1, p_short, 50.0f);
    Arc long_arc(t1, p_long, 50.0f);

    TimedToken short_token(short_arc, 1u, TypeOfNet::PetriNet);
    TimedToken long_token(long_arc, 1u, TypeOfNet::PetriNet);

    ASSERT_FLOAT_EQ(short_token.speed, 10.0f / 0.2f);
    ASSERT_FLOAT_EQ(long_token.speed, 200.0f / 0.2f);

    float dt = 0.05f;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_FALSE(short_token.update(dt));
        ASSERT_FALSE(long_token.update(dt));
    }
    ASSERT_TRUE(short_token.update(dt));
    ASSERT_TRUE(long_token.update(dt));
}
