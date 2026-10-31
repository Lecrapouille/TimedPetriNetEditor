# 🔀 Timed conflict

Two tokens, two ways of different durations: the choice made at `t = 0` changes the whole future.

![Timed conflict](TimedConflict.png)

## What it models

`P1` has two tokens and two output transitions: `T1` (its tokens reach `P2` after 5) and `T2` (after 4). At `t = 0` both can fire: this is a **conflict**, solved by *Simulation > Conflicts* (first declared, highest priority, random with a seed). `T3` then gives one token to `P0` and one to `P3`, which comes back to `P0` through `T4`: the number of tokens grows.

## What it teaches

- [Conflicts in time](../../../doc/en/06-Timed-Petri-Nets.md#7-conflicts-in-time): the policy is part of the model
- [Earliest firing](../../../doc/en/06-Timed-Petri-Nets.md#33-earliest-firing)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Timed-Petri-Nets/TimedConflict/TimedConflict.json
```

1. *Simulation > Conflicts > First declared*, press **Space**: `T1` fires twice at `t = 0` (its identifier is the lowest), `T3` fires first at `t = 5`.
2. *Random*, run it twice: the seed is in the console, the runs differ; with the seed 7, `T2` fires twice at `t = 0` and `T3` first at `t = 4`.
3. *Analysis > Properties*: the net is unbounded (`T3` gives two tokens for one), so the exploration stops; the (max,+) tab says why this net is not an event graph (`P0` has two input arcs).

---

Back to the [examples](../../README.md) · Timed Petri nets: [chapter 6](../../../doc/en/06-Timed-Petri-Nets.md)
