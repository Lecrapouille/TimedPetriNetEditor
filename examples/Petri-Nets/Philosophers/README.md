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

## Run it with the runtime

[`Philosophers.cpp`](Philosophers.cpp) runs this net with the runtime of the project ([chapter 11](../../../doc/11-Runtime.md)), on a pool of three threads: a meal is the function of `PhiN eats`, it lasts 100 ms, and the three philosophers could eat at once if the net let them. The program has **no mutex**: the tokens of `Fork` and of the two `Knife` are the locks, taken when `PhiN eats` fires and given back when `PhiN thinks` fires. A guard gives each philosopher three meals; the three are hungry at the same time, and the `Random` policy chooses who eats (with a seed, the same choices at each run). Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/Philosophers
   0 ms  Phi0 starts meal 1 (1 eating)
 102 ms  Phi1 starts meal 1 (1 eating)
 204 ms  Phi1 starts meal 2 (1 eating)
 306 ms  Phi1 starts meal 3 (1 eating)
 408 ms  Phi2 starts meal 1 (1 eating)
 510 ms  Phi0 starts meal 2 (1 eating)
 612 ms  Phi2 starts meal 2 (1 eating)
 715 ms  Phi0 starts meal 3 (1 eating)
 817 ms  Phi2 starts meal 3 (1 eating)
 919 ms  Canceled: 3, 3 and 3 meals, at most 1 philosopher eating at once
```

The program counts the philosophers eating at the same time: never more than one, as the analysis said (each pair shares a piece of cutlery). The 9 meals take 9 × 100 ms: the threads do not help, the net serializes the meals. The guard can still change, so the instance waits for it and does not end by itself: the program stops it (`Canceled`) when the meals are eaten.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
