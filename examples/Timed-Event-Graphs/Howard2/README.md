# 🔁 Howard2

The textbook timed event graph: two circuits, and the slowest one sets the pace.

![Howard2](Howard2.png)

## What it models

Four transitions, drawn compactly: each arc between two transitions is a hidden place, with its tokens (the disk with a number) and its duration. `P0` (from `T0` to `T2`) has 2 tokens and the duration 5. Two circuits go through `T0` and `T2`:

| Circuit | Durations | Tokens | Mean |
|---------|-----------|:------:|------|
| `T0 → T2 → T1 → T0` | 5 + 3 + 5 = 13 | 2 | **6.5** |
| `T0 → T2 → T3 → T0` | 5 + 1 + 1 = 7 | 2 | 3.5 |

The cycle time is the largest mean: λ = 6.5.

## What it teaches

- [The compact drawing](../../../doc/07-Timed-Event-Graphs.md#2-the-compact-drawing)
- [The dater equations, by hand](../../../doc/07-Timed-Event-Graphs.md#32-by-hand-on-howard2)
- [Howard's algorithm, by hand](../../../doc/08-Max-Plus-Algebra.md#62-by-hand-on-howard2)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Timed-Event-Graphs/Howard2/Howard2.json
```

1. *Analysis > (max,+) algebra > Critical circuit*: λ = 6.5, the circuit `T0 → T2 → T1` is red on the net, the bias is `(0, 5, 8, 1)`.
2. *Equations*: `T0(k) = max(1 + T3(k), 5 + T1(k))`, `T2(k) = max(5 + T0(k - 2))` ...
3. *Gantt*: the earliest dates, `T0: 8 8 21 21`: two firings every 13, the cyclicity is 2.
4. Double click the arc `T2 → T3`, give it the duration 10: the other circuit becomes critical, λ = (5 + 10 + 1) / 2 = 8.
5. Press **Space**: the tokens on their way are green disks; they join the disk of the waiting tokens near the end of the arc.

---

Back to the [examples](../../README.md) · Timed event graphs: [chapter 7](../../../doc/07-Timed-Event-Graphs.md) · (max,+) algebra: [chapter 8](../../../doc/08-Max-Plus-Algebra.md)
