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
| [MicroServices.cpp](MicroServices.cpp) | services | a request to a fake HTTP service; its answer is injected; a **timeout** is a transition with a delay in conflict with the answer, going to a place of error |
| [Benchmark.cpp](Benchmark.cpp) | desktop | the workload of the example *performance* of Symmetri: its default net, or one of its PNML nets, timed (§ below) |
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
transitions without function. It times one run, as Symmetri does, then the
mean of many runs. Build the libraries optimized, as Symmetri is (`-O3`):

```sh
for d in src/PetriNet src/IO src/Runtime; do make -C $d COMPILATION_MODE=release BUILD=build-release; done
g++ -std=c++20 -O3 -DNDEBUG -Isrc -Iexternal -Iexternal/json/include examples/runtime/Benchmark.cpp \
    -Lbuild-release -lTimedPetriRuntime -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$PWD/build-release -o Benchmark
./Benchmark                               # the default net
./Benchmark Symmetri/nets/n1.pnml 100     # a net of Symmetri, 100 runs
```

Then, on the same machine, build Symmetri with its examples
(`cmake -DBUILD_EXAMPLES=ON`) and run `./Performance [net.pnml]`.

On a laptop (Linux, g++ 16, `-O3`): the default net runs in 1.1 µs (12 µs
the first time), about 5 million firings per second; `n1.pnml` and `sat.pnml`
loop without end (a cycle of transitions without function nor duration: the
runtime stops them after `max_firings_per_date` firings); `T50startP0.pnml`
only has a transition without input place, which waits for `inject()`. The
big `auto.pnml` (1921 transitions, 29152 arcs) runs in 0.42 ms: each instance
checks all its transitions once, then only the ones around the places that
changed.
