# 🔁 Deadlock

Two robots, two tools, taken in opposite orders: a deadlock.

![Deadlock](Deadlock.png)

## What it models

Robot A takes tool 1 then tool 2; robot B takes tool 2 then tool 1. Each releases both when its work is done.

## What it teaches

- [Shared resources and deadlocks](../../../doc/en/03-Petri-Nets.md#62-shared-resources-and-the-dining-philosophers)
- [Deadlock detection, shortest path](../../../doc/en/04-Analysis.md#31-deadlock)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Petri-Nets/Deadlock/Deadlock.json
```

1. Press **Space**, fire `A takes 1` then `B takes 2`: nothing is green any more.
2. Stop, open *Analysis > Properties*: deadlock found, after `T0 (A takes 1), T3 (B takes 2)`. *Replay the path in the Journal*.
3. Fix it: make robot B take tool 1 first (move the arcs), and check again.

## Properties

```text
$ build/linux-gcc-debug/bin/tpne-check --allow-deadlock examples/Petri-Nets/Deadlock/Deadlock.json
examples/Petri-Nets/Deadlock/Deadlock.json:
Net: Deadlock (Petri net, 8 places, 6 transitions)
Reachable markings: 6
Deadlock: yes, marking (P0=0, P1=1, P2=0, P3=0, P4=0, P5=0, P6=1, P7=0) after firing T0 (A takes 1), T3 (B takes 2)
Dead transitions: none
Bounded: yes (1-bounded)
Live: no, not live: T0 (A takes 1), T1 (A takes 2), T2 (A releases), T3 (B takes 2), T4 (B takes 1), T5 (B releases)
```

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
