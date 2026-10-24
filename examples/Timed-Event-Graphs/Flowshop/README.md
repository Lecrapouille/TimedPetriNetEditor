# 🏭 Flowshop

A workshop where every piece visits the machines in the same order: how fast is it, and which machine slows it down?

![Flowshop](simple.png)

## What it models

[`simple.flowshop`](simple.flowshop) has 3 pieces and 2 machines, as a table of processing times:

```text
pieces: P1 P2 P3
M1: 2.0 3.0 1.0      # M1 works 2 on P1, 3 on P2, 1 on P3
M2: 1.0 2.0 1.5
```

The editor builds its timed event graph: `M1_P2` is the operation of `M1` on `P2`; each row is the loop of a machine (one token: one piece at a time), each column the loop of a piece (one token: on one machine at a time). [`complex.flowshop`](complex.flowshop) has 8 machines and 6 pieces.

## What it teaches

- [The tutorial *How fast is my factory?*](../../../doc/07-Timed-Event-Graphs.md#6-tutorial-how-fast-is-my-factory): cycle time, bottleneck, Gantt chart
- [The flowshop format](../../../doc/15-File-Formats.md#43-flowshop)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Timed-Event-Graphs/Flowshop/simple.flowshop
```

1. *Analysis > (max,+) algebra > Critical circuit*: λ = 6, the loop of `M1` (2 + 3 + 1) is red: `M1` is the bottleneck (`M2` works 4.5 per cycle).
2. *Gantt*: one row per machine, one color per piece (the legend names them). Only the machines have rows: the other transitions (`M1`, `P1`, `out_...`) take no time.
3. Press **Space**: the *Journal* gives the same dates as the Gantt chart, `M1_P1` at 0, `M1_P2` and `M2_P1` at 2 ...
4. Edit the file: `nm: 2 1` (two machines `M1`), open it again: λ = 5, and the critical circuit is now the loop of the piece `P2` (3 + 2): buying a faster `M2` would not help.
5. Open `complex.flowshop`: λ = 16.95, and the critical circuit is the column of `Piece3`, not a machine.

---

Back to the [examples](../../README.md) · Timed event graphs: [chapter 7](../../../doc/07-Timed-Event-Graphs.md) · (max,+) algebra: [chapter 8](../../../doc/08-Max-Plus-Algebra.md)
