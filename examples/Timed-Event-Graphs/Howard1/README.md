# 🧮 Howard1

Two transitions, four places of two tokens each: three circuits, which one is critical?

![Howard1](Howard1.png)

## What it models

Each transition has a loop on itself (`T0`: duration 1, `T1`: duration 4) and they are joined by two places (`T0 → T1`: 3, `T1 → T0`: 2). Every place holds 2 tokens. The circuits:

| Circuit | Durations | Tokens | Mean |
|---------|-----------|:------:|------|
| `T0 → T0` | 1 | 2 | 0.5 |
| `T1 → T1` | 4 | 2 | **2** |
| `T0 → T1 → T0` | 3 + 2 = 5 | 4 | 1.25 |

## What it teaches

- [Howard's algorithm](../../../doc/en/08-Max-Plus-Algebra.md#6-howards-algorithm): the cycle time is the largest mean of the circuits
- [The dater equations](../../../doc/en/07-Timed-Event-Graphs.md#31-the-dater-form): two tokens give the shift `k - 2`

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Timed-Event-Graphs/Howard1/Howard1.json
```

1. *Analysis > (max,+) algebra > Critical circuit*: λ = 2, the loop of `T1` (4 for 2 tokens).
2. *Equations*: `T0(k) = max(1 + T0(k - 2), 2 + T1(k - 2))`, `T1(k) = max(3 + T0(k - 2), 4 + T1(k - 2))`: every token shift is `k - 2`.
3. *Gantt*: `T0: 0 0 2 2`, `T1: 0 0 4 4`.

---

Back to the [examples](../../README.md) · Timed event graphs: [chapter 7](../../../doc/en/07-Timed-Event-Graphs.md) · (max,+) algebra: [chapter 8](../../../doc/en/08-Max-Plus-Algebra.md)
