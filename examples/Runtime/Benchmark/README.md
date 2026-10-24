# ⏲️ Benchmark

This runtime against [Symmetri](https://github.com/thorstink/Symmetri), on the same machine and the same nets.

## What it measures

`Benchmark` runs the workload of the example
[performance](https://github.com/thorstink/Symmetri/tree/main/examples/performance)
of Symmetri: without argument the net `t0: Pa + Pb -> Pc`, `t1: 2 Pc -> 2 Pb`
with synchronous functions; with a PNML file of Symmetri (`nets/*.pnml`) its
transitions without function. `SymmetriBenchmark` is the example of Symmetri
timed the same way: the net is ready before the clock starts, the run alone
is timed, once (as the example of Symmetri does), then the mean of many runs.

```sh
# This project, optimized as Symmetri is (-O3)
for d in src/PetriNet src/IO src/Runtime; do make -C $d COMPILATION_MODE=release BUILD=build-release; done
g++ -std=c++20 -O3 -DNDEBUG -Isrc -Iexternal -Iexternal/json/include examples/Runtime/Benchmark/Benchmark.cpp \
    -Lbuild-release -lTimedPetriRuntime -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$PWD/build-release -o Benchmark

# Symmetri (https://github.com/thorstink/Symmetri, with its submodules)
cmake -S Symmetri -B Symmetri/build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON && cmake --build Symmetri/build
g++ -std=c++17 -O3 -DNDEBUG -ISymmetri/symmetri/include examples/Runtime/Benchmark/SymmetriBenchmark.cpp \
    -LSymmetri/build/symmetri -lsymmetri -Wl,-rpath,$PWD/Symmetri/build/symmetri -pthread -o SymmetriBenchmark

./Benchmark                     ; ./SymmetriBenchmark                       # the default net
./Benchmark Symmetri/nets/auto.pnml 50 ; ./SymmetriBenchmark Symmetri/nets/auto.pnml 50
```

Measured on the same machine (AMD Ryzen 7 9700X, Linux, g++ 16.2, `-O3`,
Symmetri at commit `22e47a4`), mean of the runs:

| Net | Firings | TimedPetriNetEditor | Symmetri |
|-----|:-------:|--------------------:|---------:|
| default (2 transitions, 6 tokens) | 6 | 1.0 µs | 0.6 µs |
| `hello_world.pnml`, `PT1.pnml` | 1 | 0.19 µs | 0.13 µs |
| `auto.pnml` (1921 transitions, 29152 arcs) | 12 / 19 | **0.43 ms** (36 µs a firing) | 2.8 ms (150 µs a firing) |
| `n1.pnml`, `sat.pnml` (a cycle without duration) | 10000 | stopped: 2.1 ms, 3.3 ms | never ends: its journal fills the memory |
| `T50startP0.pnml` (a transition without input) | | waits for `inject()` | 0 firings |

- On small nets Symmetri is faster, by 0.1 to 0.5 µs a run: the journal of
  this runtime keeps the names of the instance and of the transition in each
  entry, Symmetri their indices.
- On a big net this runtime is 4 to 6 times faster: an instance checks all
  its transitions once, then only the ones around the places that changed.
- In a conflict, the two do not choose the same transition (here, the
  highest priority then the first declared; Symmetri its own order): on
  `auto.pnml` they follow different paths, of 12 and 19 firings, both ending
  in a deadlock. Compare the time of a firing.
- A cycle of transitions without function nor duration fires for ever: this
  runtime stops it after `Options::max_firings_per_date` firings (10000) and
  ends the instance with `Failed`.

## What it teaches

- [Compared with Symmetri](../../../doc/11-Runtime.md#10-compared-with-symmetri)
- [The PNML of Symmetri](../../../doc/15-File-Formats.md#41-pnml-and-pnml-for-symmetri): the editor reads and writes it, to compare on the same nets

---

Back to the [runtime examples](../README.md) · [all the examples](../../README.md) · The runtime: [chapter 11](../../../doc/11-Runtime.md)
