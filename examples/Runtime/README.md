# ⚙️ Examples of the runtime

A net drawn in the editor (or built in the code), run by a program that gives a function to each transition ([chapter 11](../../doc/11-Runtime.md)). Build the project first, then:

```sh
make -C examples                # build/examples/
make -C examples run            # run them
```

| Example | Target | What it shows |
|---------|--------|---------------|
| 🤖 [PickAndPlace](PickAndPlace/README.md) | desktop | [`PickAndPlace.json`](PickAndPlace/PickAndPlace.json) run on a pool of threads; the result of `grasp` (Success, Failed) colors the token, the arcs choose `place` or `retry`; the journal in JSON lines; `Debugger.cpp` publishes it to the editor (ZeroMQ, built when libzmq is found) |
| 🌐 [MicroServices](MicroServices/README.md) | services | a request to a fake HTTP service; its answer is injected; a **timeout** is a transition with a delay in conflict with the answer; the net saved as JSON, PNML and Graphviz, the journal as CSV and XES (process mining) |
| 🚦 [TrafficLight](TrafficLight/README.md) | micro-controller | `setup()` and `loop()` as in a sketch: one thread, `step()` with the time of `millis()`, synchronous functions, a pedestrian button injected, no journal |
| 🐢 [Ros2Node](Ros2Node/README.md) | ROS 2 | a timer of the node calls `step()`; functions send ROS actions, their results come back by `inject()` (needs `rclcpp`, not built here) |
| ⏲️ [Benchmark](Benchmark/README.md) | desktop | the workload of the example *performance* of Symmetri, timed against Symmetri on the same machine |

The nets of the other families have programs too (⚙️ in [the gallery](../README.md)): [Coffee](../Petri-Nets/Coffee/README.md), [LandingGear](../Petri-Nets/LandingGear/README.md), [EmergencyCalls](../Petri-Nets/EmergencyCalls/README.md), [Philosophers](../Petri-Nets/Philosophers/README.md), [ProducerConsumer](../Petri-Nets/ProducerConsumer/README.md), [Colors](../Extended-Petri-Nets/Colors/README.md), [BoundedBuffer](../Extended-Petri-Nets/BoundedBuffer/README.md), [Priorities](../Extended-Petri-Nets/Priorities/README.md).

`make` also compiles the runtime with `-fno-exceptions -fno-rtti`, as on a board. On a micro-controller, build the net in `setup()` (no file system), keep `Options::log` false and use the `InlineExecutor`: after `setup()` the runtime only allocates for the requests of `inject()` and for the tokens on their way along arcs with a duration.

---

Back to the [examples](../README.md) · The runtime: [chapter 11](../../doc/11-Runtime.md) · The debugger: [chapter 13](../../doc/13-Debugging.md)
