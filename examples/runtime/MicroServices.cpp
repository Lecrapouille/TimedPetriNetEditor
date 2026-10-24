// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: example of the runtime (micro-services, timeout).
//
// An order calls a payment service. The service answers by a message
// (inject("paid")) unless it is too slow: the transition "timeout", in
// conflict with "paid" on the token of Waiting, has a delay of 0.5 s and
// takes the token first. This is the pattern of a timeout: a timed
// transition in competition with the slow one, going to a place of error.
//
//   (New) -> [request] -> (Waiting) -> [paid] <- inject   -> (Paid)
//                             \-------> [timeout] 0.5 s    -> (Error)
//=============================================================================

#include "PetriNet/PetriNet.hpp"
#include "Runtime/Runtime.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace tpne;

//------------------------------------------------------------------------------
static Net makeOrder()
{
    Net net(TypeOfNet::TimedPetriNet);
    net.name = "Order";
    Place& created = net.addPlace(1u, "New");
    Place& waiting = net.addPlace(0u, "Waiting");
    Place& paid = net.addPlace(0u, "Paid");
    Place& failed = net.addPlace(0u, "Error");
    Place& answer = net.addPlace(0u, "Answer");
    Transition& request = net.addTransition("request");
    Transition& response = net.addTransition("paid");
    Transition& confirm = net.addTransition("confirm");
    Transition& timeout = net.addTransition("timeout");
    timeout.delay = 0.5f;
    net.addArc(created, request); net.addArc(request, waiting);
    net.addArc(response, answer); // input transition: the message of the service
    net.addArc(waiting, confirm); net.addArc(answer, confirm); net.addArc(confirm, paid);
    net.addArc(waiting, timeout); net.addArc(timeout, failed);
    return net;
}

//------------------------------------------------------------------------------
static Color order(double const service_delay)
{
    Net const net = makeOrder();
    Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(2u));
    std::shared_ptr<Instance> instance = runtime.instantiate("order", {}, { { "Paid", Success } });

    // The fake HTTP service answers after its delay, on its own thread.
    std::thread service;
    runtime.on("request", [&runtime, &service, instance, service_delay]() {
        service = std::thread([&runtime, instance, service_delay]() {
            std::this_thread::sleep_for(std::chrono::duration<double>(service_delay));
            runtime.inject(*instance, "paid");
        });
    });

    // Paid is the goal; Error ends the order too.
    while (runtime.step(*instance) && (instance->marking().count(3u) == 0u))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (instance->isRunning())
    {
        runtime.cancel(*instance);
        runtime.run(*instance);
    }
    service.join(); // a late answer is ignored
    return (instance->marking().count(2u) > 0u) ? Success : Failed;
}

//------------------------------------------------------------------------------
int main()
{
    Color const fast = order(0.1);
    std::cout << "fast service: " << ((fast == Success) ? "paid" : "timeout") << std::endl;
    Color const slow = order(1.0);
    std::cout << "slow service: " << ((slow == Success) ? "paid" : "timeout") << std::endl;
    return ((fast == Success) && (slow == Failed)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
