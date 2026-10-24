# Examples of the runtime

A net drawn in the editor, run by a program that gives a function to each
transition (chapter [10](../../doc/10-Library.md#6-run-your-functions-the-runtime)).
Build the project first, then:

```sh
make -C examples/runtime        # build/examples/
make -C examples/runtime run    # run them
```

| Example | Target | What it shows |
|---------|--------|---------------|
| [PickAndPlace.cpp](PickAndPlace.cpp) | desktop | [`PickAndPlace.json`](../../data/examples/PickAndPlace.json) run on a pool of threads; the result of `grasp` (Success, Failed) colors the token, the arcs choose `place` or `retry`; the journal in JSON lines |
| [MicroServices.cpp](MicroServices.cpp) | services | a request to a fake HTTP service; its answer is injected; a **timeout** is a transition with a delay in conflict with the answer, going to a place of error; the net saved as JSON, PNML and Graphviz (libTimedPetriIO), the journal as CSV and XES (process mining) |
| [Benchmark.cpp](Benchmark.cpp) | desktop | the workload of the example *performance* of Symmetri: its default net, or one of its PNML nets, timed (§ below) |
| [Debugger.cpp](Debugger.cpp) | desktop | the pick and place over and over, publishing its journal on ZeroMQ: open `PickAndPlace.json` in the editor, *Debug > Connect*, and watch it run (built when libzmq is found) |
| [TrafficLight.cpp](TrafficLight.cpp) | micro-controller | `setup()` and `loop()` as in a sketch: one thread, `step()` with the time of `millis()`, synchronous functions, a pedestrian button injected, no journal |
| [Ros2Node.cpp](Ros2Node.cpp) | ROS 2 | a timer of the node calls `step()`; functions send ROS actions, their results come back by `inject()` (needs `rclcpp`, not built here) |

`make` also compiles the runtime with `-fno-exceptions -fno-rtti`, as on a
board. On a micro-controller, build the net in `setup()` (no file system),
keep `Options::log` false and use the `InlineExecutor`: after `setup()` the
runtime only allocates for the requests of `inject()` and for the tokens on
their way along arcs with a duration.

## Benchmark against Symmetri

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
g++ -std=c++20 -O3 -DNDEBUG -Isrc -Iexternal -Iexternal/json/include examples/runtime/Benchmark.cpp \
    -Lbuild-release -lTimedPetriRuntime -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$PWD/build-release -o Benchmark

# Symmetri (https://github.com/thorstink/Symmetri, with its submodules)
cmake -S Symmetri -B Symmetri/build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON && cmake --build Symmetri/build
g++ -std=c++17 -O3 -DNDEBUG -ISymmetri/symmetri/include examples/runtime/SymmetriBenchmark.cpp \
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
