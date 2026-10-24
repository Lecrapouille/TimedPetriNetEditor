# 🔀 Fork and join

Two tasks run in parallel, then wait for each other.

![Fork and join](ForkJoin.png)

## What it models

`Fork` starts two branches; `Task A` and `Task B` are independent; `Join` waits for both; `Restart` closes the loop.

## What it teaches

- [Concurrency](../../../doc/03-Petri-Nets.md#51-concurrency-independent-events)
- [Fork, join and loop patterns](../../../doc/03-Petri-Nets.md#6-patterns-of-modeling)
- [T-invariant: the whole cycle](../../../doc/04-Analysis.md#7-t-invariants)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/ForkJoin/ForkJoin.json
```

1. Press **Space**, fire `Fork`: both tasks are enabled.
2. Fire `Task B` first, then `Task A`: the order does not matter, they are concurrent.
3. `Join` is enabled only when both branches are done: fire it, then `Restart`.
4. *Analysis > Properties*: one T-invariant, `Fork + Task A + Task B + Join + Restart`, the cycle of the net.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
