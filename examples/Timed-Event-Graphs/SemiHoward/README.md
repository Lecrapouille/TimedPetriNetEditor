# 📄 SemiHoward

A small net in the `.teg` format, with two strongly connected components.

![SemiHoward](SemiHoward.png)

## What it models

[`SemiHoward.teg`](SemiHoward.teg), one line per hidden place, `<from> <to>: <duration> <tokens>`:

```text
TimedEventGraph 3 5

0 1:   0   1
1 0:   1   0
2 0:   0   0
2 1:   1   0
2 2:   2   1
```

`T0` and `T1` form a circuit of mean 1; `T2`, a loop of mean 2, feeds them. A transition goes at the pace of the slowest component upstream of it: here 2 for everyone.

## What it teaches

- [The `.teg` format](../../../doc/en/07-Timed-Event-Graphs.md#7-the-teg-file-format)
- [Nets that are not strongly connected](../../../doc/en/08-Max-Plus-Algebra.md#53-what-if-the-net-is-not-strongly-connected)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Timed-Event-Graphs/SemiHoward/SemiHoward.teg
```

1. *Analysis > (max,+) algebra > Critical circuit*: two components, with the circuits `T0 → T1 → T0` and `T2 → T2`.
2. *Gantt*: `T0: 2 4 6 8`, `T1: 1 3 5 7`, `T2: 0 2 4 6`: every transition goes at the pace 2 of `T2`. The table of the cycle times gives 1 to `T0` and `T1` (the mean of their own component): trust the earliest dates there.

---

Back to the [examples](../../README.md) · Timed event graphs: [chapter 7](../../../doc/en/07-Timed-Event-Graphs.md) · (max,+) algebra: [chapter 8](../../../doc/en/08-Max-Plus-Algebra.md)
