# 🚦 German traffic lights

A German traffic light goes green, yellow, red, then **red and yellow
together** before green again. This GRAFCET is a loop of four steps driven by
time only: each receptivity is a time operator (`3s/X0`: true 3 s after
step 0 became active).

![German traffic lights](GermanTrafficLights.png)

**Notions**: sequence and loop
([chapter 9 §4.1](../../../doc/09-GRAFCET.md#41-sequence-and-loop)), time
operators ([§5.2](../../../doc/09-GRAFCET.md#52-time)), several actions on one
step and several actions driving the same output
([§6](../../../doc/09-GRAFCET.md#6-actions)), the Outputs window and the
timing diagram ([§9](../../../doc/09-GRAFCET.md#9-inputs-outputs-and-the-control-panel)).

**Try it**

```sh
./build/TimedPetriNetEditor examples/GRAFCET/GermanTrafficLights/GermanTrafficLights.json
```

1. Press **Space**: the green light is on, and the timer `x / 3 s` counts
   under `3s/X0`.
2. Watch the Outputs window: step 3 has two actions, `N Red LED` and
   `N Yellow LED`, and step 2 also drives `Red LED`: an output is on while
   one of its actions is on.
3. Open the *Timing diagram* tab: the four steps follow each other, the red
   light stays on during steps 2 and 3 (4 s).
4. Change the speed to **x10** in the toolbar: the cycle of 8 s lasts 0.8 s.

There is no input: the control panel says so. Credit: the idea comes from a
video of the YAWL User Group.
