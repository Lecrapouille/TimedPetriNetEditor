# 🔌 Inputs and outputs

Sources and sinks: the inputs `u` and the outputs `y` of a system, and a queue that grows because an input is too fast.

![Inputs and outputs](InputsOutputs.png)

## What it models

`T0` and `T3` have no input place: they are **sources** (clients, pieces, messages coming from outside). A source fires again as soon as the tokens of its last firing have arrived: `T0` every 3 time units (the duration of its arc), `T3` every 2. `T2` and `T5` have no output place: they are **sinks**. `T4` takes the tokens of `T3` from `P2` only when `P5` has a token, fed by `T1`, which goes at the pace of `T0`.

## What it teaches

- [Sources and sinks](../../../doc/en/06-Timed-Petri-Nets.md#63-sources-and-sinks-the-inputs-and-outputs-of-a-system)
- [The queue that grows](../../../doc/en/06-Timed-Petri-Nets.md#64-the-queue-that-grows): bounded without time, unbounded with time, or the opposite
- [Inputs and outputs in the (max,+) algebra](../../../doc/en/07-Timed-Event-Graphs.md#5-inputs-and-outputs)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Timed-Petri-Nets/InputsOutputs/InputsOutputs.json
```

1. Press **Space** and wait: the sources are drawn in orange while their tokens are on their way.
2. Watch `P2`: `T3` adds a token every 2, `T4` takes one every 3. At `t = 12`, `P2` holds 2 tokens and gains one every 6.
3. *Analysis > Properties*: `P4 + P5 = 2`; without time the sources make `P0` to `P3` unbounded.

---

Back to the [examples](../../README.md) · Timed Petri nets: [chapter 6](../../../doc/en/06-Timed-Petri-Nets.md)
