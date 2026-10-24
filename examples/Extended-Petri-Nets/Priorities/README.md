# 🚨 Priorities

Priorities in a timed Petri net: the ambulance crosses first.

![Priorities](Priorities.png)

## What it models

A car and an ambulance wait for the same crossing (a conflict on `Crossing free`). `Ambulance crosses` has priority 1, `Car crosses` priority 0. The arcs have durations: this is a timed Petri net, it runs by itself.

## What it teaches

- [Priorities](../../../doc/05-Extended-Petri-Nets.md#5-priorities)
- [Conflicts in timed nets](../../../doc/03-Petri-Nets.md#52-conflict-a-choice)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Extended-Petri-Nets/Priorities/Priorities.json
```

1. Choose *Simulation > Conflicts > Highest priority*, press **Space**: the ambulance crosses first, every time.
2. Stop, choose *Random*: the car crosses first about half of the time; the seed is in the console, *Replay the last simulation* gives the same run.
3. The *Transitions* tab of the inspector shows and changes the priorities.

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/05-Extended-Petri-Nets.md)
