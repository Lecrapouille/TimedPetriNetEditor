# 🐢 Ros2Node

The runtime inside a ROS 2 node: the net drives ROS actions, no thread waits.

## What it models

[`Ros2Node.cpp`](Ros2Node.cpp) loads a net (the parameter `net`, `cell.json` by default) and runs it in the executor of ROS:

- a **timer** of the node calls `step()` every 10 ms: the marking lives in the thread of ROS, with the clock of ROS (`Options::clock`);
- the function of `grasp` **sends the goal of a ROS action and returns at once** (the function is synchronous, the `InlineExecutor` runs it);
- the result of the action comes back by `inject()` on input transitions (`grasp_ok`, `grasp_ko`): the net goes on when the robot answers.

## What it teaches

- [Targets: ROS 2](../../../doc/en/11-Runtime.md#6-targets)
- [`run()` or `step()`](../../../doc/en/11-Runtime.md#4-run-or-step): `step()` for a loop that belongs to someone else
- [Sources and injected events](../../../doc/en/11-Runtime.md#32-sources-and-injected-events)

## Build it

It needs `rclcpp`, so it is not built by `make -C examples`. Put it in a colcon package that links `libTimedPetriRuntime`, `libTimedPetriIO` and `libTimedPetriNet` (and their headers, `src/` of this project), with C++20. The node can publish its journal to the editor too ([chapter 13](../../../doc/en/13-Debugging.md)).

---

Back to the [runtime examples](../README.md) · [all the examples](../../README.md) · The runtime: [chapter 11](../../../doc/en/11-Runtime.md)
