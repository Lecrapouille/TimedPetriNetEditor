# 🍝 Dining philosophers

Three philosophers share one fork and two knives.

![Dining philosophers](Philosophers.png)

## What it models

Phi0 eats with the fork and the first knife, Phi1 with the fork and the second knife, Phi2 with both knives. Each piece of cutlery is a place with one token; a philosopher takes both pieces at once, so there is no deadlock.

## What it teaches

- [Shared resources](../../../doc/03-Petri-Nets.md#62-shared-resources-and-the-dining-philosophers)
- [P-invariants: one per resource](../../../doc/04-Analysis.md#62-what-p-invariants-tell)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/Philosophers/Philosophers.json
```

1. Press **Space**, let `Phi0 eats`: `Phi1 eats` and `Phi2 eats` turn grey.
2. Who can eat at the same time? Nobody: each pair of philosophers shares a piece.
3. *Analysis > Properties*: click on each P-invariant, the nodes of the resource are marked on the net.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
