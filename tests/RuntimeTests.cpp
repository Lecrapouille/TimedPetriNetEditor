// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "Runtime/Runtime.hpp"
#include "Runtime/SubNet.hpp"
#include "Runtime/Journal.hpp"

#include <atomic>
#include <chrono>
#include <thread>

using namespace ::tpne;

//------------------------------------------------------------------------------
//! \brief The machine of the roadmap: Ready -> detect -> Seen -> grasp ->
//! Held; Held (Success) -> place -> Done; Held (Failed) -> retry -> Seen.
static Net pickAndPlace()
{
    Net net(TypeOfNet::PetriNet);
    Place& ready = net.addPlace(1u);
    Place& seen = net.addPlace(0u);
    Place& held = net.addPlace(0u);
    Place& done = net.addPlace(0u);
    ready.caption = "Ready"; seen.caption = "Seen"; held.caption = "Held"; done.caption = "Done";
    Transition& detect = net.addTransition();
    Transition& grasp = net.addTransition();
    Transition& place = net.addTransition();
    Transition& retry = net.addTransition();
    detect.caption = "detect"; grasp.caption = "grasp"; place.caption = "place"; retry.caption = "retry";
    net.addArc(ready, detect); net.addArc(detect, seen);
    net.addArc(seen, grasp); net.addArc(grasp, held);
    net.addArc(held, place); net.addArc(place, done);
    net.addArc(held, retry); net.addArc(retry, seen);
    net.findArc(held, place)->color = Success;
    net.findArc(held, retry)->color = Failed;
    return net;
}

//------------------------------------------------------------------------------
static size_t count(Instance const& instance, std::string const& transition, LogEntry::Event event)
{
    size_t n = 0u;
    for (auto const& entry : instance.log())
        n += ((entry.transition == transition) && (entry.event == event)) ? 1u : 0u;
    return n;
}

//------------------------------------------------------------------------------
TEST(TestRuntime, PickAndPlace)
{
    Net const net = pickAndPlace();
    Runtime runtime(net);
    int detections = 0;
    int grasps = 0;
    ASSERT_TRUE(runtime.on("detect", [&detections]() { ++detections; }));
    ASSERT_TRUE(runtime.on("grasp", [&grasps]() { return ++grasps >= 3; })); // fails twice
    ASSERT_TRUE(runtime.on("place", []() {}));
    ASSERT_FALSE(runtime.on("unknown", []() {}));
    // "retry" has no function: it fires at once.
    ASSERT_EQ(runtime.unregisteredTransitions(), std::vector<std::string>({ "retry" }));

    auto instance = runtime.instantiate("order_42", {}, { { "Done", Success } });
    ASSERT_EQ(runtime.run(*instance), Success);
    ASSERT_EQ(instance->result(), Success);
    ASSERT_EQ(detections, 1);
    ASSERT_EQ(grasps, 3);
    ASSERT_EQ(count(*instance, "grasp", LogEntry::Event::Completed), 3u);
    ASSERT_EQ(count(*instance, "retry", LogEntry::Event::Scheduled), 2u);
    ASSERT_EQ(instance->marking().count(3u, Success), 1u);
    ASSERT_EQ(instance->log().front().instance, "order_42");
}

//------------------------------------------------------------------------------
TEST(TestRuntime, ThreadPoolAndInstances)
{
    Net const net = pickAndPlace();
    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(4u));
    std::atomic<int> grasps{0};
    runtime.on("detect", []() { std::this_thread::sleep_for(std::chrono::milliseconds(2)); });
    runtime.on("grasp", [&grasps]() { return (++grasps % 2) == 0; });
    runtime.on("place", []() {});

    // Several instances of the same net, run by several threads.
    std::vector<std::shared_ptr<Instance>> instances;
    for (int i = 0; i < 8; ++i)
        instances.push_back(runtime.instantiate("case" + std::to_string(i), {}, { { "Done", Success } }));
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};
    for (auto& instance : instances)
        threads.emplace_back([&runtime, instance, &successes]() {
            if (runtime.run(*instance) == Success)
                ++successes;
        });
    for (auto& thread : threads)
        thread.join();
    ASSERT_EQ(successes.load(), 8);
}

//------------------------------------------------------------------------------
TEST(TestRuntime, DeadlockedAndFailed)
{
    Net const net = pickAndPlace();

    // Without goal, the instance ends when nothing can happen any more.
    Runtime runtime(net);
    runtime.on("grasp", []() { return true; });
    auto instance = runtime.instantiate("a");
    ASSERT_EQ(runtime.run(*instance), Deadlocked);

    // A Failed token nobody takes: the instance fails.
    Net failing = pickAndPlace();
    failing.removeArc(*failing.findArc(failing.transitions()[3], failing.places()[1]));
    failing.removeNode(failing.transitions()[3]); // no retry
    Runtime runtime2(failing);
    runtime2.on("grasp", []() { return false; });
    auto instance2 = runtime2.instantiate("b", {}, { { "Done", Success } });
    ASSERT_EQ(runtime2.run(*instance2), Failed);
}

//------------------------------------------------------------------------------
TEST(TestRuntime, GuardsAndInjectedInputs)
{
    // Button (source) -> Ready -> detect -> Seen
    Net net(TypeOfNet::PetriNet);
    Place& ready = net.addPlace(0u);
    Place& seen = net.addPlace(0u);
    ready.caption = "Ready"; seen.caption = "Seen";
    Transition& button = net.addTransition();
    Transition& detect = net.addTransition();
    button.caption = "button"; detect.caption = "detect";
    net.addArc(button, ready); net.addArc(ready, detect); net.addArc(detect, seen);

    Runtime runtime(net);
    bool open = false;
    runtime.guard("detect", [&open](Context const& context) {
        return open && (context.tokens("Ready") > 0u);
    });
    auto instance = runtime.instantiate("c", {}, { { "Seen", Success } });

    // Nothing happens until the button is pushed (sources never deadlock).
    ASSERT_TRUE(runtime.step(*instance, 0.0));
    ASSERT_TRUE(runtime.step(*instance, 1.0));
    runtime.inject(*instance, "button");
    ASSERT_TRUE(runtime.step(*instance, 2.0));
    ASSERT_EQ(instance->marking().count(0u), 1u); // the guard is closed
    open = true;
    ASSERT_FALSE(runtime.step(*instance, 3.0));
    ASSERT_EQ(instance->result(), Success);
}

//------------------------------------------------------------------------------
TEST(TestRuntime, CancelRunningFunctions)
{
    // A long function on a pool, canceled from another thread.
    struct Long
    {
        std::shared_ptr<std::atomic<bool>> canceled = std::make_shared<std::atomic<bool>>(false);
    };
    Net const net = pickAndPlace();
    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(2u));
    auto canceled = std::make_shared<std::atomic<bool>>(false);
    runtime.on("detect", [canceled]() {
        while (!*canceled)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return Canceled;
    });
    auto instance = runtime.instantiate("d", {}, { { "Done", Success } });
    std::thread killer([&runtime, instance, canceled]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        runtime.cancel(*instance);
        *canceled = true;
    });
    ASSERT_EQ(runtime.run(*instance), Canceled);
    killer.join();
}

//------------------------------------------------------------------------------
TEST(TestRuntime, TimedArcsAndDelays)
{
    // Timed: T0 -(2)-> P1 -> T1 (delay 1) -> P2
    Net net(TypeOfNet::TimedPetriNet);
    Place& p0 = net.addPlace(1u);
    Place& p1 = net.addPlace(0u);
    Place& p2 = net.addPlace(0u);
    p2.caption = "End";
    Transition& t0 = net.addTransition();
    Transition& t1 = net.addTransition();
    t1.delay = 1.0f;
    net.addArc(p0, t0); net.addArc(t0, p1, 2.0f);
    net.addArc(p1, t1); net.addArc(t1, p2);

    Runtime runtime(net);
    auto instance = runtime.instantiate("e", {}, { { "End", Success } });
    ASSERT_TRUE(runtime.step(*instance, 10.0)); // start at 10
    ASSERT_EQ(instance->marking().count(1u), 0u); // on its way
    ASSERT_TRUE(runtime.step(*instance, 12.5)); // arrived at 2, delay until 3
    ASSERT_EQ(instance->marking().count(1u), 1u);
    ASSERT_FALSE(runtime.step(*instance, 13.0));
    ASSERT_EQ(instance->result(), Success);
    ASSERT_DOUBLE_EQ(instance->log().back().date, 3.0);
}

//------------------------------------------------------------------------------
TEST(TestRuntime, PrioritiesAndZeno)
{
    // P0 -> T0 (priority 1) -> P1, P0 -> T1 -> P2
    Net net(TypeOfNet::PetriNet);
    Place& p0 = net.addPlace(1u);
    net.addPlace(0u);
    net.addPlace(0u);
    Transition& t0 = net.addTransition();
    Transition& t1 = net.addTransition();
    net.addArc(p0, t0); net.addArc(t0, net.places()[1]);
    net.addArc(p0, t1); net.addArc(t1, net.places()[2]);
    t1.priority = 1;
    Runtime runtime(net);
    auto instance = runtime.instantiate("f");
    ASSERT_EQ(runtime.run(*instance), Deadlocked);
    ASSERT_EQ(instance->marking().count(2u), 1u);

    // P0 -> T0 -> P0: no function, no duration.
    Net loop(TypeOfNet::PetriNet);
    Place& q = loop.addPlace(1u);
    Transition& t = loop.addTransition();
    loop.addArc(q, t); loop.addArc(t, q);
    Runtime::Options options;
    options.max_firings_per_date = 50u;
    Runtime looping(loop, std::make_shared<InlineExecutor>(), options);
    auto zeno = looping.instantiate("g");
    ASSERT_EQ(looping.run(*zeno), Failed);
    ASSERT_NE(zeno->error().find("without duration"), std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief Start -> [pick] -> Picked; Picked (Success) -> [ship] -> Shipped;
//! Picked (Failed) -> [alarm] -> Alarm. The function of "pick" is the net of
//! pickAndPlace().
static Net parent()
{
    Net net(TypeOfNet::PetriNet);
    Place& start = net.addPlace(1u);
    Place& picked = net.addPlace(0u);
    Place& shipped = net.addPlace(0u);
    Place& alarm = net.addPlace(0u);
    start.caption = "Start"; picked.caption = "Picked"; shipped.caption = "Shipped"; alarm.caption = "Alarm";
    Transition& pick = net.addTransition();
    Transition& ship = net.addTransition();
    Transition& raise = net.addTransition();
    pick.caption = "pick"; ship.caption = "ship"; raise.caption = "alarm";
    net.addArc(start, pick); net.addArc(pick, picked);
    net.addArc(picked, ship); net.addArc(ship, shipped);
    net.addArc(picked, raise); net.addArc(raise, alarm);
    net.findArc(picked, ship)->color = Success;
    net.findArc(picked, raise)->color = Failed;
    return net;
}

//------------------------------------------------------------------------------
TEST(TestRuntime, NestedNets)
{
    Net const child_net = pickAndPlace();
    Net const parent_net = parent();

    // The sub net succeeds: the parent ships.
    auto child = std::make_shared<Runtime>(child_net);
    child->on("grasp", []() { return true; });
    SubNet sub(child, { { "Done", Success } });
    Runtime runtime(parent_net);
    runtime.on("pick", sub);
    auto order = runtime.instantiate("order", {}, { { "Shipped", Success } });
    ASSERT_EQ(runtime.run(*order), Success);
    ASSERT_EQ(sub.instances().size(), 1u);
    ASSERT_EQ(sub.instances()[0]->result(), Success);

    // The sub net deadlocks (no grasp ever succeeds and no retry): its end is
    // the result of "pick", the parent raises the alarm.
    Net failing = pickAndPlace();
    failing.removeNode(failing.transitions()[3]);
    auto child2 = std::make_shared<Runtime>(failing);
    child2->on("grasp", []() { return Deadlocked; });
    Runtime runtime2(parent_net);
    runtime2.on("pick", SubNet(child2, { { "Done", Success } }));
    auto order2 = runtime2.instantiate("order2", {}, { { "Alarm", Failed } });
    ASSERT_EQ(runtime2.run(*order2), Deadlocked);
    ASSERT_EQ(order2->marking().count(1u, Deadlocked), 1u); // Picked holds the result
}

//------------------------------------------------------------------------------
TEST(TestRuntime, CancelNestedNets)
{
    Net const child_net = pickAndPlace();
    Net const parent_net = parent();
    auto child = std::make_shared<Runtime>(child_net, std::make_shared<ThreadPoolExecutor>(1u));
    auto stop = std::make_shared<std::atomic<bool>>(false);
    child->on("detect", [stop]() {
        while (!*stop)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
    SubNet sub(child, { { "Done", Success } });
    Runtime runtime(parent_net, std::make_shared<ThreadPoolExecutor>(1u));
    runtime.on("pick", sub);
    auto order = runtime.instantiate("order", {}, { { "Shipped", Success } });
    std::thread killer([&runtime, order, stop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        runtime.cancel(*order); // cancels the sub net too
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        *stop = true;
    });
    ASSERT_EQ(runtime.run(*order), Canceled);
    killer.join();
    ASSERT_EQ(sub.instances().size(), 1u);
    ASSERT_EQ(sub.instances()[0]->result(), Canceled);
}

//------------------------------------------------------------------------------
TEST(TestRuntime, JournalInJsonLines)
{
    Net const net = pickAndPlace();
    std::vector<std::string> written;
    Runtime::Options options;
    options.journal = [&written, &net](LogEntry const& entry) {
        written.push_back(toJsonLine(entry, net.colors));
    };
    Runtime runtime(net, std::make_shared<InlineExecutor>(), options);
    int grasps = 0;
    runtime.on("grasp", [&grasps]() { return ++grasps >= 2; });
    auto instance = runtime.instantiate("order_7", {}, { { "Done", Success } });
    ASSERT_EQ(runtime.run(*instance), Success);

    // Written as it goes, the same as the journal of the instance.
    std::string const text = toJsonLines(instance->log(), net.colors);
    ASSERT_EQ(written.size(), instance->log().size());
    ASSERT_NE(text.find("{\"instance\":\"order_7\",\"transition\":\"grasp\",\"event\":\"Completed\","
                        "\"result\":\"Failed\",\"date\":0"), std::string::npos) << text;

    std::vector<LogEntry> journal;
    ASSERT_STREQ(fromJsonLines(text, net.colors, journal).c_str(), "");
    ASSERT_EQ(journal.size(), instance->log().size());
    for (size_t i = 0u; i < journal.size(); ++i)
    {
        ASSERT_EQ(journal[i].transition, instance->log()[i].transition);
        ASSERT_EQ(journal[i].event, instance->log()[i].event);
        ASSERT_EQ(journal[i].result, instance->log()[i].result);
    }
    ASSERT_STRNE(fromJsonLines("{\"event\":\"Oops\",\"transition\":\"a\"}", net.colors, journal).c_str(), "");
}

//------------------------------------------------------------------------------
TEST(TestRuntime, RestoreAfterCrash)
{
    Net const net = pickAndPlace();

    // The program stopped while "grasp" was running: the journal has its
    // Scheduled and Started, not its Completed.
    Runtime first(net, std::make_shared<ThreadPoolExecutor>(1u));
    auto stop = std::make_shared<std::atomic<bool>>(false);
    first.on("grasp", [stop]() {
        while (!*stop)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    });
    auto crashed = first.instantiate("order_9", {}, { { "Done", Success } });
    for (int i = 0; (i < 1000) && (count(*crashed, "grasp", LogEntry::Event::Started) == 0u); ++i)
    {
        first.step(*crashed, 0.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::vector<LogEntry> const journal = crashed->log();
    ASSERT_EQ(count(*crashed, "grasp", LogEntry::Event::Completed), 0u);
    first.cancel(*crashed);
    *stop = true;
    first.run(*crashed);

    // Restart: the function of "grasp" is called again.
    Runtime second(net);
    int grasps = 0;
    second.on("grasp", [&grasps]() { ++grasps; return true; });
    auto restarted = second.restore("order_9", journal, Runtime::Recovery::Restart, {}, { { "Done", Success } });
    ASSERT_EQ(restarted->marking().count(0u), 0u); // Ready was consumed
    ASSERT_EQ(second.run(*restarted), Success);
    ASSERT_EQ(grasps, 1);

    // Fail: "grasp" ends with Failed, "retry" takes the token, grasp again.
    grasps = 0;
    auto failed = second.restore("order_9", journal, Runtime::Recovery::Fail, {}, { { "Done", Success } });
    ASSERT_EQ(second.run(*failed), Success);
    ASSERT_EQ(grasps, 1);
    ASSERT_EQ(count(*failed, "retry", LogEntry::Event::Scheduled), 1u);
}
