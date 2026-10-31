# 🔒 Mutual exclusion

Two processes and one shared resource: one at a time.

![Mutual exclusion](MutualExclusion.png)

## What it models

Each process is `Idle` or `Critical`; entering the critical section takes the token of `Mutex`, leaving gives it back.

## What it teaches

- [Mutual exclusion pattern](../../../doc/en/03-Petri-Nets.md#61-mutual-exclusion)
- [Reachability graph by hand](../../../doc/en/04-Analysis.md#21-by-hand-mutual-exclusion)
- [State equation and P-invariants by hand](../../../doc/en/04-Analysis.md#5-the-incidence-matrix-and-the-state-equation)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Petri-Nets/MutualExclusion/MutualExclusion.json
```

1. Press **Space**, fire `Enter A`: `Enter B` turns grey.
2. Fire `Leave A`: `Enter B` is enabled again.
3. *Analysis > Properties*: 3 markings, live, and the P-invariant `Critical A + Mutex + Critical B = 1`, the proof of the exclusion.
4. Add a token to `Mutex` (**+**): the invariant becomes `= 2`, both processes can be critical.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
