# 🖼️ Examples

One folder per example, grouped by family of nets. Each folder holds the
net (`.json`, `.flowshop` or `.teg`), its picture, a README explaining what
it models and what to try, and sometimes a C++ program that runs it with
the runtime ([chapter 11](../doc/11-Runtime.md)). Most nets also carry a
**description**, written on the canvas above the net: it tells you what to
do and what to look at.

Open any of them from the root of the project:

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/Philosophers/Philosophers.json
```

and build and run the programs with `make -C examples` then, for example,
`./build/examples/Coffee`. 📝 marks the small "textbook" examples that teach
one notion each, ⚙️ the examples with a program.

## 🔵 Petri nets

Chapters [3](../doc/03-Petri-Nets.md) and [4](../doc/04-Analysis.md).

| | Example | Notions |
|-|---------|---------|
| <img src="Petri-Nets/FirstNet/FirstNet.png" width="220"> | 📝 [FirstNet](Petri-Nets/FirstNet/README.md) | the firing rule on the smallest net |
| <img src="Petri-Nets/ForkJoin/ForkJoin.png" width="220"> | 📝 [ForkJoin](Petri-Nets/ForkJoin/README.md) | concurrency, synchronization |
| <img src="Petri-Nets/Conflict/Conflict.png" width="220"> | 📝 [Conflict](Petri-Nets/Conflict/README.md) | choice between two transitions |
| <img src="Petri-Nets/MutualExclusion/MutualExclusion.png" width="220"> | 📝 [MutualExclusion](Petri-Nets/MutualExclusion/README.md) | a shared resource, a P-invariant |
| <img src="Petri-Nets/Deadlock/Deadlock.png" width="220"> | 📝 [Deadlock](Petri-Nets/Deadlock/README.md) | a deadlock and the path to it |
| <img src="Petri-Nets/ProducerConsumer/ProducerConsumer.png" width="220"> | ⚙️ [ProducerConsumer](Petri-Nets/ProducerConsumer/README.md) | a buffer, an unbounded place; restart after a crash |
| <img src="Petri-Nets/Philosophers/Philosophers.png" width="220"> | ⚙️ [Philosophers](Petri-Nets/Philosophers/README.md) | shared resources, invariants; tokens as locks of threads |
| <img src="Petri-Nets/Coffee/Coffee.png" width="220"> | ⚙️ [Coffee](Petri-Nets/Coffee/README.md) | synchronization, choice by the result of a function |
| <img src="Petri-Nets/TrafficLights/TrafficLights.png" width="220"> | [TrafficLights](Petri-Nets/TrafficLights/README.md) | safety by a conflict: never two greens |
| <img src="Petri-Nets/LandingGear/LandingGear.png" width="220"> | ⚙️ [LandingGear](Petri-Nets/LandingGear/README.md) | a net as a safety layer; read arcs |
| <img src="Petri-Nets/EmergencyCalls/EmergencyCalls.png" width="220"> | ⚙️ [EmergencyCalls](Petri-Nets/EmergencyCalls/README.md) | operators as resources, colored calls |
| <img src="Petri-Nets/ElevatorControl/ElevatorControl.png" width="220"> | [ElevatorControl](Petri-Nets/ElevatorControl/README.md) | an interlock: move only with the door shut |
| <img src="Petri-Nets/ChainSaw/ChainSaw.png" width="220"> | [ChainSaw](Petri-Nets/ChainSaw/README.md) | safety conditions read by a transition |
| <img src="Petri-Nets/SmarthomeSafety/SmarthomeSafety.png" width="220"> | [SmarthomeSafety](Petri-Nets/SmarthomeSafety/README.md) | a counter of resources: at most 3 of 4 |
| <img src="Petri-Nets/FourRoadJunctions/FourRoadJunctions.png" width="220"> | [FourRoadJunctions](Petri-Nets/FourRoadJunctions/README.md) | sources and sinks of a road network |

## 🧪 Extended Petri nets

Chapter [5](../doc/05-Extended-Petri-Nets.md).

| | Example | Notions |
|-|---------|---------|
| <img src="Extended-Petri-Nets/Weights/Weights.png" width="220"> | 📝 [Weights](Extended-Petri-Nets/Weights/README.md) | arc weights: 2 H2 + O2 → 2 H2O |
| <img src="Extended-Petri-Nets/Capacity/Capacity.png" width="220"> | 📝 [Capacity](Extended-Petri-Nets/Capacity/README.md) | a place of capacity 3 |
| <img src="Extended-Petri-Nets/Inhibitor/Inhibitor.png" width="220"> | 📝 [Inhibitor](Extended-Petri-Nets/Inhibitor/README.md) | the zero test |
| <img src="Extended-Petri-Nets/Reset/Reset.png" width="220"> | 📝 [Reset](Extended-Petri-Nets/Reset/README.md) | emptying a place |
| <img src="Extended-Petri-Nets/Priorities/Priorities.png" width="220"> | 📝 ⚙️ [Priorities](Extended-Petri-Nets/Priorities/README.md) | a conflict solved by priorities, or at random with a seed |
| <img src="Extended-Petri-Nets/Colors/Colors.png" width="220"> | 📝 ⚙️ [Colors](Extended-Petri-Nets/Colors/README.md) | the color of a token chooses its way |
| <img src="Extended-Petri-Nets/ColoredPhilosophers/ColoredPhilosophers.png" width="220"> | 📝 ⚙️ [ColoredPhilosophers](Extended-Petri-Nets/ColoredPhilosophers/README.md) | colored initial tokens fold places; simulation and runtime agree |
| <img src="Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.png" width="220"> | ⚙️ [BoundedBuffer](Extended-Petri-Nets/BoundedBuffer/README.md) | all the extensions together |

## ⏱️ Timed Petri nets

Chapter [6](../doc/06-Timed-Petri-Nets.md).

| | Example | Notions |
|-|---------|---------|
| <img src="Timed-Petri-Nets/Durations/Durations.png" width="220"> | 📝 [Durations](Timed-Petri-Nets/Durations/README.md) | tokens travel along the arcs |
| <img src="Timed-Petri-Nets/Timeout/Timeout.png" width="220"> | 📝 [Timeout](Timed-Petri-Nets/Timeout/README.md) | a delay in conflict with an answer |
| <img src="Timed-Petri-Nets/TimedConflict/TimedConflict.png" width="220"> | [TimedConflict](Timed-Petri-Nets/TimedConflict/README.md) | a conflict in a timed net |
| <img src="Timed-Petri-Nets/InputsOutputs/InputsOutputs.png" width="220"> | [InputsOutputs](Timed-Petri-Nets/InputsOutputs/README.md) | sources and sinks |
| <img src="Timed-Petri-Nets/Unbounded/Unbounded.png" width="220"> | [Unbounded](Timed-Petri-Nets/Unbounded/README.md) | a place that grows for ever |

## 🔁 Timed event graphs and (max,+) algebra

Chapters [7](../doc/07-Timed-Event-Graphs.md) and [8](../doc/08-Max-Plus-Algebra.md).

| | Example | Notions |
|-|---------|---------|
| <img src="Timed-Event-Graphs/Howard2/Howard2.png" width="220"> | 📝 [Howard2](Timed-Event-Graphs/Howard2/README.md) | cycle time 6.5, critical circuit |
| <img src="Timed-Event-Graphs/Howard1/Howard1.png" width="220"> | [Howard1](Timed-Event-Graphs/Howard1/README.md) | the Howard algorithm |
| <img src="Timed-Event-Graphs/EventGraph/EventGraph.png" width="220"> | [EventGraph](Timed-Event-Graphs/EventGraph/README.md) | the event graph of *Synchronization and Linearity* |
| <img src="Timed-Event-Graphs/JPQ/JPQ.png" width="220"> | [JPQ](Timed-Event-Graphs/JPQ/README.md) | inputs, outputs, the (max,+) linear system |
| <img src="Timed-Event-Graphs/Flowshop/simple.png" width="220"> | [Flowshop](Timed-Event-Graphs/Flowshop/README.md) | a factory: cycle time, bottleneck, Gantt chart |
| <img src="Timed-Event-Graphs/SemiHoward/SemiHoward.png" width="220"> | [SemiHoward](Timed-Event-Graphs/SemiHoward/README.md) | the `.teg` format |
| <img src="Timed-Event-Graphs/SemiNetherlands/SemiNetherlands.png" width="220"> | [SemiNetherlands](Timed-Event-Graphs/SemiNetherlands/README.md) | a bigger `.teg` net |

## 🔘 GRAFCET and GEMMA

Chapters [9](../doc/09-GRAFCET.md) and [10](../doc/10-GEMMA.md).

| | Example | Notions |
|-|---------|---------|
| <img src="GRAFCET/GermanTrafficLights/GermanTrafficLights.png" width="220"> | 📝 [GermanTrafficLights](GRAFCET/GermanTrafficLights/README.md) | a sequence with timers |
| <img src="GRAFCET/ExclusiveChoice/ExclusiveChoice.png" width="220"> | 📝 [ExclusiveChoice](GRAFCET/ExclusiveChoice/README.md) | OR divergence, exclusive receptivities |
| <img src="GRAFCET/ParallelSequences/ParallelSequences.png" width="220"> | 📝 [ParallelSequences](GRAFCET/ParallelSequences/README.md) | AND divergence and convergence |
| <img src="GRAFCET/GrafcetActions/GrafcetActions.png" width="220"> | [GrafcetActions](GRAFCET/GrafcetActions/README.md) | the action qualifiers |
| <img src="GRAFCET/GEMMA-Dryer/GEMMA-Dryer.png" width="220"> | [GEMMA-Dryer](GRAFCET/GEMMA-Dryer/README.md) | three GRAFCETs, forcing orders, a control panel |

## ⚙️ Runtime

Chapters [11](../doc/11-Runtime.md), [12](../doc/12-Code-Generation.md) and
[13](../doc/13-Debugging.md). The programs next to the nets above (⚙️) are
runtime examples too.

| | Example | Notions |
|-|---------|---------|
| <img src="Runtime/PickAndPlace/PickAndPlace.png" width="220"> | ⚙️ [PickAndPlace](Runtime/PickAndPlace/README.md) | a robot: results that choose the branch, the editor as a debugger |
| | ⚙️ [MicroServices](Runtime/MicroServices/README.md) | a service answering late, a timeout, journals for process mining |
| | ⚙️ [TrafficLight](Runtime/TrafficLight/README.md) | a micro-controller: `setup()`, `loop()`, no thread |
| | ⚙️ [Ros2Node](Runtime/Ros2Node/README.md) | a ROS 2 node |
| | ⚙️ [Benchmark](Runtime/Benchmark/README.md) | this runtime against Symmetri |

## 🔄 Make the pictures again

The pictures are taken by the editor itself:

```sh
tools/screenshots.sh            # all of them
tools/screenshots.sh Coffee     # the ones whose path contains Coffee
```

A file `screenshot.args` next to a net gives other options than the net
alone (`-w` for the whole window, `-r -d 3` after 3 s of simulation).
