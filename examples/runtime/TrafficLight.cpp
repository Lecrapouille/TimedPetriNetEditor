// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: example of the runtime as on a micro-controller.
//
// One thread, no executor thread, no exception: step() is called by loop()
// as an Arduino sketch would do, with the time of millis(). The functions
// are synchronous and switch the lamps. A pedestrian button (a transition
// without input place) is injected by the program.
//
// Build for a board with a C++17 standard library (ESP32, RP2040, STM32
// with arduino-pico or STM32duino): compile src/PetriNet/PetriNet.cpp,
// Firing.cpp, Engine.cpp and src/Runtime/Runtime.cpp with -fno-exceptions
// -fno-rtti, build the net in setup() (no file system needed), keep
// Options::log false (no allocation by the journal).
//=============================================================================

#include "PetriNet/PetriNet.hpp"
#include "Runtime/Runtime.hpp"

#include <cstdio>
#include <cstdlib>

using namespace tpne;

//------------------------------------------------------------------------------
// What a sketch has: the lamps and the clock.
static char const* g_lamp = "off";
static unsigned long g_millis = 0u;
static unsigned long millis() { return g_millis; }
static void lamp(char const* color)
{
    g_lamp = color;
    std::printf("%6lu ms: %s\n", millis(), color);
}

//------------------------------------------------------------------------------
// Red -> [go] 3 s -> Green -> [slow] 3 s -> Orange -> [stop] 1 s -> Red, and
// a pedestrian button shortening the green.
struct Light
{
    Net net{ TypeOfNet::TimedPetriNet };
    std::unique_ptr<Runtime> runtime;
    std::shared_ptr<Instance> instance;
};

static Light g_light;

//------------------------------------------------------------------------------
static void light(char const* name, char const* color)
{
    g_light.runtime->on(name, [color]() { lamp(color); });
}

//------------------------------------------------------------------------------
void setup()
{
    Net& net = g_light.net;
    Place& red = net.addPlace(0.0f, 0.0f, 1u);
    Place& green = net.addPlace(0.0f, 0.0f, 0u);
    Place& orange = net.addPlace(0.0f, 0.0f, 0u);
    Place& request = net.addPlace(0.0f, 0.0f, 0u);
    request.capacity = 1u;
    Transition& go = net.addTransition(0.0f, 0.0f);
    Transition& slow = net.addTransition(0.0f, 0.0f);
    Transition& stop = net.addTransition(0.0f, 0.0f);
    Transition& button = net.addTransition(0.0f, 0.0f);
    Transition& hurry = net.addTransition(0.0f, 0.0f);
    go.caption = "go"; slow.caption = "slow"; stop.caption = "stop";
    button.caption = "button"; hurry.caption = "hurry";
    go.delay = 3.0f; slow.delay = 3.0f; stop.delay = 1.0f;
    hurry.priority = 1;
    net.addArc(red, go); net.addArc(go, green);
    net.addArc(green, slow); net.addArc(slow, orange);
    net.addArc(orange, stop); net.addArc(stop, red);
    net.addArc(button, request);
    net.addArc(green, hurry); net.addArc(request, hurry); net.addArc(hurry, orange);

    Runtime::Options options;
    options.log = false;
    options.clock = []() { return double(millis()) / 1000.0; };
    g_light.runtime = std::make_unique<Runtime>(net, std::make_shared<InlineExecutor>(), options);
    light("go", "green");
    light("slow", "orange");
    light("hurry", "orange (pedestrian)");
    light("stop", "red");
    g_light.instance = g_light.runtime->instantiate("light");
    lamp("red");
}

//------------------------------------------------------------------------------
void loop()
{
    g_light.runtime->step(*g_light.instance);
}

//------------------------------------------------------------------------------
// The host program plays the board: 20 s of loop(), a pedestrian at 4.5 s.
int main()
{
    setup();
    for (g_millis = 0u; g_millis <= 20000u; g_millis += 10u)
    {
        if (g_millis == 4500u)
        {
            std::printf("%6lu ms: pedestrian button\n", millis());
            g_light.runtime->inject(*g_light.instance, "button");
        }
        loop();
    }
    return g_light.instance->isRunning() ? EXIT_SUCCESS : EXIT_FAILURE;
}
