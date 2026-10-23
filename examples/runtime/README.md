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
| [TrafficLight.cpp](TrafficLight.cpp) | micro-controller | `setup()` and `loop()` as in a sketch: one thread, `step()` with the time of `millis()`, synchronous functions, a pedestrian button injected, no journal |
| [Ros2Node.cpp](Ros2Node.cpp) | ROS 2 | a timer of the node calls `step()`; functions send ROS actions, their results come back by `inject()` (needs `rclcpp`, not built here) |

`make` also compiles the runtime with `-fno-exceptions -fno-rtti`, as on a
board. On a micro-controller, build the net in `setup()` (no file system),
keep `Options::log` false and use the `InlineExecutor`: after `setup()` the
runtime only allocates for the requests of `inject()` and for the tokens on
their way along arcs with a duration.
