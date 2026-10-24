# ⏱️ Timed Petri Net Editor

[![Non regression tests](https://github.com/Lecrapouille/TimedPetriNetEditor/actions/workflows/non_regression.yml/badge.svg)](https://github.com/Lecrapouille/TimedPetriNetEditor/actions/workflows/non_regression.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

**Draw it. Run it. Find out why it is slow. Then run your code with it.**

A coffee machine, a crossroads with traffic lights, three philosophers
who share one fork and two knives, a factory, a dryer with an emergency stop,
a robot that picks objects: they all do several things at the same time, and
they all wait for each other somewhere. TimedPetriNetEditor lets you draw
them with the mouse, watch them run, press their buttons, and get the answer
to the question everybody asks: *how fast can it go, and what slows it
down?* Then it runs your own functions with the same net, and shows you your
program running.

![A GEMMA running](doc/pics/Editor-GEMMA-running.png)

*A dryer controlled by three GRAFCETs: press its buttons on the control
panel, watch the motors turn and the oven heat.*

## ✨ What you can do with it

🎲 **Play with a system until you understand it.** Click on a transition and
the tokens move. Three philosophers, one fork, two knives: who can eat at
the same time, and who has to wait? Add a token, remove an arc, and see what
changes.

🏭 **Measure a factory before you build it.** Open a flowshop (pieces,
machines, processing times) and the editor gives, without simulating, the
time between two products, the machine that slows everything down, and the
schedule of every machine as a Gantt chart. Then press Space and watch the
simulation follow the same dates.

🔘 **Run a machine without the machine.** Draw a GRAFCET the way automation
is taught (IEC 60848): steps, conditions such as `Start . /EStop`, timers
such as `3s/X2`, actions, forcing orders between graphs (GEMMA). Then drive
it from a control panel with real push buttons, a mushroom emergency stop, a
mode selector and a thermometer, and read its timing diagram.

🔍 **Know before it breaks.** The editor finds the deadlocks of any net (and
the shortest path to them, replayed step by step), the transitions that can
never fire, the places that grow without bound, the invariants;
`tpne-check` does the same in your CI.

⚙️ **Run your code with it.** In your program, a transition is a function:
the runtime calls it when the transition fires and its result (success,
failure, a color of yours) chooses what comes next, on a board or on a pool
of threads. The editor writes the skeleton of this program.

🐞 **Watch your program run.** Connect the editor to your program, or to the
GRAFCET code generated for your Arduino: the tokens move in the editor as
your program runs, and its journal can be replayed at any date.

📦 **Take it with you.** Export to SVG or LaTeX for your report, to
Graphviz, PNML, GreatSPN or Draw.io; read the nets of Symmetri and GreatSPN;
generate the C++ code of your GRAFCETs for an Arduino; save journals for
process mining (CSV, XES); drive the editor from Python over ZeroMQ, or use
its engine in your own C++ or Julia program.

| | | |
|:-:|:-:|:-:|
| ![Coffee](data/examples/pics/Coffee.png) | ![Philosophers](data/examples/pics/Philosophers.png) | ![Traffic lights](data/examples/pics/TrafficLights.png) |
| ☕ a coffee machine | 🍝 the dining philosophers | 🚦 traffic lights |
| ![Flowshop](doc/pics/Editor-Flowshop.png) | ![Critical circuit](doc/pics/Circuit01.png) | ![Emergency calls](doc/pics/911.png) |
| 🏭 a flowshop factory | 🐢 its bottleneck in red | 🚑 [an emergency call center (video)](https://youtu.be/hOhunzgFpcA) |
| ![Properties](doc/pics/Editor-Properties.png) | ![Debugger](doc/pics/Editor-Debugger.png) | ![GRAFCET](doc/pics/Editor-GEMMA.png) |
| 🔍 deadlocks, bounds, invariants | 🐞 a robot program followed live | 🔘 three GRAFCETs of a GEMMA |

All of them are in [data/examples](data/examples/README.md). An online
version is in progress:
🌐 [try it in your browser](https://lecrapouille.github.io/TimedPetriNetEditor/TimedPetriNetEditor.html).

## 🤔 Why this one

| | This editor | Most Petri net editors | GRAFCET tools |
|-|:-:|:-:|:-:|
| Cycle time, bottleneck, Gantt chart, (max,+) equations of a timed event graph | ✅ | rarely | ❌ |
| GRAFCET by the standard: edges, timers, all action qualifiers, forcing orders, several graphs | ✅ | ❌ | ✅ |
| A control panel with real buttons, a timing diagram, timers you can see count | ✅ | ❌ | with a PLC |
| Draw with the mouse only: no mode, no palette, the status bar tells you what a click does | ✅ | rarely | rarely |
| Deadlocks, bounds, liveness, invariants, in the editor and in a CI | ✅ | some | ❌ |
| Your functions run by the net (runtime), the editor as a visual debugger | ✅ | ❌ | with a PLC |
| C++ library, Julia, remote control, exports, C++ code generation | ✅ | rarely | ❌ |
| Free, open source, Linux, macOS, Windows | ✅ | varies | rarely |

The runtime does the job of [Symmetri](https://github.com/thorstink/Symmetri)
and was measured against it on the same machine: Symmetri is faster on tiny
nets (0.6 µs against 1.0 µs for 6 firings), this runtime 4 to 6 times faster
on a net of 1921 transitions, and it stops a cycle that would fire for ever
([the numbers](examples/runtime/README.md#benchmark-against-symmetri)).

## 🚀 Try it in two minutes

```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1 --recursive
cd TimedPetriNetEditor/
make download-external-libs
make compile-external-libs
make -j8
```

(The remote control and the debugger need ZeroMQ:
`sudo dnf install zeromq-devel`, `sudo apt install libzmq3-dev` or
`brew install zeromq`; or build without them with `make TPNE_ZEROMQ=0`.)

Then four things to try:

```sh
./build/TimedPetriNetEditor data/examples/Philosophers.json
```

🍝 Press **Space**, then click on the green transitions: a philosopher eats
when the cutlery is free, and the others wait.

```sh
./build/TimedPetriNetEditor data/examples/simple.flowshop
```

🏭 *Analysis > Critical circuit*: the factory makes a set of pieces every 6
time units, and machine M1 is the bottleneck. *Analysis > Gantt chart*: who
works when.

```sh
./build/TimedPetriNetEditor data/examples/GEMMA-Dryer.json
```

🔘 Press **Space**, then on the control panel: Rearm, Ack, wait 3 s, Ack
again, wait for 80 °C, Start. Then hit the emergency stop.

```sh
make -C examples/runtime
./build/examples/Debugger &
./build/TimedPetriNetEditor -c tcp://localhost:5556 data/examples/PickAndPlace.json
```

🐞 A robot program picks and places objects; the editor follows it: the
*Journal* lists what its functions do, the net shows its tokens.

To draw your own: the **left** mouse button draws (a click adds a place, a
drag from a node draws an arc), the **right** one moves, the status bar
always says what a click would do. `F1` is the help.

## 📚 Documentation

[The documentation](doc/README.md) is written for beginners, in simple
English, with tutorials made from real runs:

| # | Chapter | You will learn |
|---|---------|----------------|
| 1 | 🧩 [Introduction](doc/01-Introduction.md) | places, transitions, tokens; the four kinds of nets |
| 2 | 🖱️ [Getting started](doc/02-Getting-Started.md) | draw and run your first net |
| 3 | ⏱️ [Timed nets and (max,+) algebra](doc/03-Timed-Nets-and-Max-Plus.md) | cycle time, bottleneck, Gantt chart of a factory |
| 4 | 🔘 [GRAFCET and GEMMA](doc/04-GRAFCET-and-GEMMA.md) | control a machine: the dryer |
| 5 | 📦 [Exports and code generation](doc/05-Exports-and-Code-Generation.md) | SVG, LaTeX, Graphviz, PNML, GRML, journals; C++ for the runtime and for GRAFCETs; the editor as a debugger |
| 6 | 📄 [File format](doc/06-File-Format.md) | the JSON file |
| 7 | 📖 [Editor reference](doc/07-Editor-Reference.md) | every window, gesture, key and menu |
| 8 | 🔢 [Julia](doc/08-Julia.md) | the editor and the (max,+) algebra from Julia |
| 9 | 📡 [Remote control](doc/09-Remote-Control.md) | drive the editor from another program; follow a running program |
| 10 | 🧰 [Use the library in C++](doc/10-Library.md) | run a GRAFCET, compute dates, run your functions, check the properties in your program |
| 11 | 🤖 [From the editor to the code](doc/11-From-Editor-To-Code.md) | a robot from the drawing to a running program, watched in the editor |

The words are in the [glossary](doc/Glossary.md), books and other tools in
the [bibliography](doc/Bibliography.md).

## 📜 The story

At INRIA, the (max,+) algebra turned the schedules of factories and trains
into simple linear equations, and the ScicosLab toolbox computed them: cycle
times, critical circuits, earliest dates. But you had to type the matrices
by hand, and ScicosLab is no longer developed. The toolbox now lives in Julia
as [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl); this editor is
its missing half: draw the net, and the matrices come by themselves.

The Petri net editors found on GitHub did not help: most of them were
abandoned years ago, needed Windows, Java or C#, or had code nobody could
extend. Then came the GRAFCET: students of automation draw GRAFCETs and
GEMMAs on paper, and to see them run they need a PLC or an expensive tool.
Today [RobotIK](https://github.com/Lecrapouille/Robotik) also drives its
simulated robot cells with the GRAFCET engine and the (max,+) algorithms of
this project.

## 🛠️ For developers

`sudo make install` installs the editor and its libraries:
`TimedPetriNet` (the nets, the simulation, GRAFCET), `TimedPetriIO` (the
file formats), `TimedPetriMaxPlus`, `TimedPetriAnalysis` and
`TimedPetriRuntime`. The unit tests: `make -C tests`, then
`../build/TimedPetriNetEditor-UnitTest` from `tests/`. In Visual Studio
Code, F5 starts the editor in the debugger.

The project may be compiled with `-ffast-math` (`PERFORMANCE_FLAGS` in
`.makefile/rules/Makefile`), which implies `-ffinite-math-only`: the compiler
then considers that `NaN` and `±Inf` never occur, so `std::isnan()` and
`std::isinf()` become `false`. To detect the (max,+) zero `-inf` or the `NaN`
"no duration" of the arcs, use the helpers of
[`src/PetriNet/SafeFloat.hpp`](src/PetriNet/SafeFloat.hpp) (`safeIsNaN`,
`safeIsNegInf`, `safeIsPosInf`, `safeIsInf`).
