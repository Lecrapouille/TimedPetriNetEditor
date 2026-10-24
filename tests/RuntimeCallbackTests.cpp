// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "Runtime/Callback.hpp"
#include "Runtime/Executor.hpp"

#include <atomic>

//------------------------------------------------------------------------------
// A type of the user: its free functions are found by ADL.
namespace robot {

struct Gripper
{
    std::shared_ptr<int> cancels = std::make_shared<int>(0);
    bool holding = false;
};

tpne::Color fire(Gripper const& gripper) { return gripper.holding ? tpne::Success : tpne::Failed; }
void cancel(Gripper const& gripper) { ++(*gripper.cancels); }
bool isSynchronous(Gripper const&) { return true; }

} // namespace robot

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestCallback, Lambdas)
{
    int calls = 0;
    Callback const nothing([&calls]() { ++calls; });
    ASSERT_EQ(nothing.fire(), Success);
    ASSERT_EQ(calls, 1);
    ASSERT_FALSE(nothing.isSynchronous());
    nothing.cancel(); // default: nothing

    Callback const boolean([]() { return false; });
    ASSERT_EQ(boolean.fire(), Failed);

    Callback const colored([]() { return Color(7u); });
    ASSERT_EQ(colored.fire(), Color(7u));

    // Copies share the function.
    Callback const copy = nothing;
    copy.fire();
    ASSERT_EQ(calls, 2);
}

//------------------------------------------------------------------------------
TEST(TestCallback, TypeOfTheUser)
{
    robot::Gripper gripper;
    Callback const callback(gripper);
    ASSERT_EQ(callback.fire(), Failed);
    ASSERT_TRUE(callback.isSynchronous());
    callback.cancel();
    callback.cancel();
    ASSERT_EQ(*gripper.cancels, 2);

    gripper.holding = true;
    ASSERT_EQ(Callback(gripper).fire(), Success);
}

//------------------------------------------------------------------------------
TEST(TestExecutor, InlineAndPool)
{
    int value = 0;
    InlineExecutor inline_executor;
    inline_executor.post([&value]() noexcept { value = 42; });
    ASSERT_EQ(value, 42); // at once

    std::atomic<int> count{0};
    {
        ThreadPoolExecutor pool(4u);
        for (int i = 0; i < 1000; ++i)
            pool.post([&count]() noexcept { ++count; });
    } // waits for the tasks
    ASSERT_EQ(count.load(), 1000);
}
