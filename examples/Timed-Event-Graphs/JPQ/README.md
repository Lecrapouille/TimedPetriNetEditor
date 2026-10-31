# 🎛️ JPQ

A timed event graph with one input `u(k)` and one output `y(k)`, named after Jean-Pierre Quadrat, drawn as a timed Petri net (with its places).

![JPQ](JPQ.png)

## What it models

Two internal transitions, `Node 0` (`x1`) and `Node 1` (`x2`), the input `u(k)` and the output `y(k)`. Its dater equations:

```text
T0(k) = max(7 + T1(k - 1), 3 + T0(k - 1))
T1(k) = max(2 + T0(k - 1), 4 + T1(k - 1), 1 + T3(k))
T2(k) = max(3 + T0(k))
```

Three circuits: `x1 → x1` (3, 1 token), `x2 → x2` (4, 1 token), `x1 → x2 → x1` (2 + 7 = 9, 2 tokens): λ = 4.5.

## What it teaches

- [The same net as a Petri net and drawn compactly](../../../doc/en/07-Timed-Event-Graphs.md#2-the-compact-drawing)
- [Inputs and outputs](../../../doc/en/07-Timed-Event-Graphs.md#5-inputs-and-outputs)
- [The matrices `D`, `A`, `B`, `C`](../../../doc/en/08-Max-Plus-Algebra.md#41-the-implicit-form)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Timed-Event-Graphs/JPQ/JPQ.json
```

1. *Analysis > (max,+) algebra > Linear system*: `D` and `A` (2 × 2), `B` (2 × 1), `C` (1 × 2); each one has *Copy as Julia*.
2. *Gantt*: `Node 0: 0 8 12 17`, `Node 1: 1 5 10 14`, `y(k): 3 11 15 20`.
3. Change the type to *Timed event graph* (toolbar): the places disappear into the arcs, the analysis is the same.
4. *File > Export > Julia*: the system for [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl) ([chapter 17](../../../doc/en/17-Julia.md)).

---

Back to the [examples](../../README.md) · Timed event graphs: [chapter 7](../../../doc/en/07-Timed-Event-Graphs.md) · (max,+) algebra: [chapter 8](../../../doc/en/08-Max-Plus-Algebra.md)
