# ⏸️ Parallel sequences

After the receptivity `m`, two sequences run at the same time: steps 1, 2, 3
on the left, steps 4, 5, 6 on the right. The right one has a loop: from step
6, `c0 . m` goes to step 7, and `c1` back to step 6; `c0 . /m` leaves it to
step 8. The last transition waits for steps 3 and 8, then goes back to step
0.

![Parallel sequences](ParallelSequences.png)

**Notions**: AND divergence and AND convergence
([chapter 9 §4.3](../../../doc/en/09-GRAFCET.md#43-parallelism-and-divergence-and-and-convergence)),
jumps and loops ([§4.4](../../../doc/en/09-GRAFCET.md#44-jumps)), a receptivity
`true` that waits only for its upstream steps.

The numbers in the squares are the identifiers of the steps (`X3` in a
receptivity); the small numbers under them are their captions, kept from
the drawing this example comes from.

**Try it**

```sh
./build/TimedPetriNetEditor examples/GRAFCET/ParallelSequences/ParallelSequences.json
```

1. Press **Space**, then turn `m` on: steps 1 and 4 are active together.
2. Move the left branch with `a1` then `a0`: it reaches step 3 and waits.
3. Move the right branch: `b1` and `c1` together (check *Apply together*
   on the control panel, change both, then **F7**: they change in the same
   instant), turn them off, then `b0`: step 6.
4. Turn `c0` on: `c0 . m` jumps to step 7. Now `c1` would go back to step 6,
   and `c0 . m`, still true, straight to step 7 again: while `c1`, `c0` and
   `m` are on together the loop turns for ever (an unstable GRAFCET, the
   console warns, [chapter 9 §12](../../../doc/en/09-GRAFCET.md#12-pitfalls)).
   So turn `m` off first: `c1` brings step 6 back, `c0 . /m` goes to step 8,
   and the AND convergence fires at once: back to step 0.
