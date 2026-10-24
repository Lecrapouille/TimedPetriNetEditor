# 🪚 Chain saw safety

A chain saw whose chain starts only when it is safe.

![Chain saw safety](ChainSaw.png)

## What it models

The chain brake, the dead man switch and the throttle are each in one of two states. `Start chain` reads `Brake released`, `Dead man switch held` and `Throttle pressed`.

## What it teaches

- [Read arcs as safety conditions](../../../doc/03-Petri-Nets.md#6-patterns-of-modeling)
- [P-invariants: each device in one state](../../../doc/04-Analysis.md#6-p-invariants)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/ChainSaw/ChainSaw.json
```

1. Press **Space**. `Start chain` is grey: the brake is on and the switch released.
2. Fire `Release brake` and `Hold switch`: `Start chain` turns green.
3. *Analysis > Properties*: 16 markings, live, 1-bounded.

## Credits

Credit: YAWL User Group, [the chain saw](https://www.youtube.com/shorts/V42WiF2Ib3w).

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
