# 📚 TimedPetriNetEditor documentation

This documentation teaches the theory of each family of nets, then shows
what the editor offers to solve its problems, with many examples. It goes
step by step: each chapter uses the words of the chapters before it. If you
never saw a Petri net, start at chapter 1 and read in order; if you know the
theory, jump to the part you need.

Every number, equation, output and picture of these pages was produced by
the editor, by `tpne-check` or by a program of [`examples/`](../examples/README.md),
not written by hand.

## 🧭 Part I — Discover

| # | Chapter | You will learn |
|---|---------|----------------|
| 1 | 🧩 [Introduction](01-Introduction.md) | What discrete event systems are, why Petri nets, the families of nets of the editor, the story of the project |
| 2 | 🚀 [Getting started](02-Getting-Started.md) | Build the editor, open an example, draw and run your first net |

## 🔵 Part II — Petri nets

| # | Chapter | You will learn |
|---|---------|----------------|
| 3 | 🔵 [Petri nets](03-Petri-Nets.md) | The formal definition, the firing rule, the marking, the patterns of modeling (sequence, parallelism, synchronization, choice, mutual exclusion, resources) |
| 4 | 🔍 [Analysis of Petri nets](04-Analysis.md) | Reachability, boundedness, liveness, deadlocks, the incidence matrix and the state equation, P- and T-invariants, and how the editor and `tpne-check` find them |
| 5 | 🧪 [Extended Petri nets](05-Extended-Petri-Nets.md) | Weights, capacities, inhibitor arcs, reset arcs, priorities, colored tokens: what each one adds, what it costs to the analysis |

## ⏱️ Part III — Time

| # | Chapter | You will learn |
|---|---------|----------------|
| 6 | ⏱️ [Timed Petri nets](06-Timed-Petri-Nets.md) | Durations and delays, how a timed net runs, timeouts, sources and sinks, conflicts in time |
| 7 | 🔁 [Timed event graphs](07-Timed-Event-Graphs.md) | Event graphs, the compact drawing, dater and counter equations, a factory (flowshop): machines, conveyors, Gantt chart |
| 8 | ➕ [The (max,+) algebra](08-Max-Plus-Algebra.md) | The algebra, its matrices, the linear systems of timed event graphs, the cycle time as an eigenvalue, Howard's algorithm and the critical circuit |

## 🏭 Part IV — Automation

| # | Chapter | You will learn |
|---|---------|----------------|
| 9 | 🔘 [GRAFCET](09-GRAFCET.md) | The standard IEC 60848: steps, receptivities, the rules of evolution, actions and their qualifiers, time, forcing; the control panel and the timing diagram |
| 10 | 🛑 [GEMMA](10-GEMMA.md) | The start and stop modes of a machine, and a dryer driven by three GRAFCETs |

## ⚙️ Part V — From the model to the program

| # | Chapter | You will learn |
|---|---------|----------------|
| 11 | ⚙️ [The runtime](11-Runtime.md) | A transition is a function of your program (the idea of Symmetri): colors of results, instances, guards, injected events, executors, journals; from the drawing to a running robot |
| 12 | 🏗️ [Code generation](12-Code-Generation.md) | The skeleton of a program for the runtime, and the C++ code of GRAFCETs for an Arduino |
| 13 | 🐞 [Debugging](13-Debugging.md) | The editor as the visual debugger of a running program, the journal, its replay, process mining |

## 📖 Part VI — Reference

| # | Chapter | Contents |
|---|---------|----------|
| 14 | 🖱️ [Editor reference](14-Editor-Reference.md) | Every window, mouse gesture, key, menu and command line option |
| 15 | 📄 [File formats](15-File-Formats.md) | The JSON file, and every format the editor reads and writes |
| 16 | 🧰 [C++ library](16-Library.md) | The libraries of the project and their API |
| 17 | 🔢 [Julia](17-Julia.md) | The editor and the (max,+) algebra from Julia |
| 18 | 📡 [Remote control](18-Remote-Control.md) | Drive the editor from another program with JSON commands over ZeroMQ |
| — | 🔤 [Glossary](Glossary.md) | Every word in bold of these pages |
| — | 📚 [Bibliography](Bibliography.md) | Books, lectures and other tools |

## 🗺️ Paths through the documentation

- **"I want to see a net run, now."** → [2](02-Getting-Started.md), then the
  [examples](../examples/README.md).
- **"I learn Petri nets."** → [1](01-Introduction.md), [3](03-Petri-Nets.md),
  [4](04-Analysis.md), [5](05-Extended-Petri-Nets.md).
- **"How fast can my factory, my trains, my process go? What slows it?"** →
  [6](06-Timed-Petri-Nets.md), [7](07-Timed-Event-Graphs.md),
  [8](08-Max-Plus-Algebra.md).
- **"I teach or learn automation."** → [9](09-GRAFCET.md),
  [10](10-GEMMA.md), [12](12-Code-Generation.md) for an Arduino.
- **"I want my program to be driven by a net."** → [3](03-Petri-Nets.md),
  [5](05-Extended-Petri-Nets.md), [11](11-Runtime.md),
  [12](12-Code-Generation.md), [13](13-Debugging.md).
- **"Can my net deadlock?"** → [4](04-Analysis.md).
- **"Which key does what?"** → [14](14-Editor-Reference.md).
