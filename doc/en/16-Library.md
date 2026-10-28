# 🧰 16. Use the library in C++


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/16-Library.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/16-Library.md)

What you will find here: how a program uses the nets without the editor: load a file, run a GRAFCET, compute the dates of a timed event graph, simulate it, draw a net in your own Dear ImGui window, run your functions, check the properties of a net.

The editor is made of these libraries (C++20):

| Library | Sources | Needs | What |
|---------|---------|-------|------|
| `libTimedPetriNet` | `src/PetriNet` | | nets, firing rule, engine, simulation, GRAFCET engine; no file format, no graphics |
| `libTimedPetriIO` | `src/IO` | Net | the file formats: `loadFromFile()`, `saveToFile()`, importers and exporters ([chapter 5](05-Exports-and-Code-Generation.md)) |
| `libTimedPetriMaxPlus` | `src/MaxPlus` | Net, IO | (max,+) algebra, Howard, critical circuit, linear systems, equations, flowshops, the Julia export |
| `libTimedPetriAnalysis` | `src/Analysis` | Net | reachability and coverability graphs, deadlocks, bounds, liveness, invariants (§8) |
| `libTimedPetriRuntime` | `src/Runtime` | Net | the runtime: a transition is a function of your program (§6) |
| `libTimedPetriGUI` | `src/Editor` | all | the editor |

A program running a net on a board needs only `libTimedPetriNet` and `libTimedPetriRuntime`: it builds its net in the code (§6) or the editor writes this code (*File > Export > C++ functions for the runtime*).

[RobotIK](https://github.com/Lecrapouille/Robotik) drives its robot cells with the first one.

---

## 1. Build against the library

After `make`, the libraries are in `build/`. A program `main.cpp`:

```sh
g++ -std=c++20 -Isrc -Iexternal/json/include -Iexternal main.cpp \
    -Lbuild -lTimedPetriMaxPlus -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$PWD/build -o main
```

`-lTimedPetriMaxPlus` only for §3 (cycle time, dates of the (max,+) equations). Call `registerMaxPlusFormats()` once to load and save `.flowshop` files and save `.jl` files with `loadFromFile()` and `saveToFile()`.

A project built with Make can compile the sources instead: define `TPNE_DIR` and include `TimedPetriNet.mk`, which gives `TPNE_SOURCES` (nets, formats, (max,+)), `TPNE_CORE_SOURCES` (the nets only), `TPNE_RUNTIME_SOURCES`, `TPNE_ANALYSIS_SOURCES`, `TPNE_INCLUDES` and `TPNE_CXXFLAGS` (and `TPNE_VIEWER_SOURCES` for §5).

## 2. Run a GRAFCET

The dryer of [chapter 4](04-GRAFCET-and-GEMMA.md#3-tutorial-the-gemma-of-a-dryer), without the editor. The program writes the inputs, calls `step(dt)` once per cycle, and reads the steps and the outputs:

```c++
#include "PetriNet/GrafcetEngine.hpp"
#include "IO/Imports/Imports.hpp"
#include <iostream>
using namespace tpne;

int main()
{
    std::vector<Net> nets;
    std::cout << importAllNetsFromJSON(nets, "data/examples/GEMMA-Dryer.json"); // error, or nothing
    GrafcetEngine e;
    e.graphs({ &nets[0], &nets[1], &nets[2] }); // they run together
    e.start();                                  // false on errors: see e.errors()
    auto show = [&](char const* what) {
        std::cout << what << "  t=" << e.time() << "  steps:";
        for (Net* n : e.graphs())
            for (auto& p : n->places()) if (p.tokens) std::cout << " " << n->name << ":X" << p.id;
        std::cout << "  outputs:";
        for (auto& o : e.outputs()) if (o.second) std::cout << " " << o.first;
        std::cout << "\n";
    };
    auto press = [&](char const* in) { e.setInput(in, 1); e.step(0.1f); e.setInput(in, 0); e.step(0.1f); };
    e.step(0.1f); show("start");
    press("Rearm"); show("Rearm");
    press("Ack"); show("Ack");
    for (int i = 0; i < 30; ++i) e.step(0.1f); show("3 s later");
    press("Ack"); show("Ack (Auto)");
    e.setInput("Temperature", 85); e.step(0.1f); show("Temperature = 85");
    press("Start"); show("Start");
    press("Stop"); show("Stop");
    for (int i = 0; i < 30; ++i) e.step(0.1f); show("3 s later");
}
```

```text
start  t=0.1  steps: Production:X0 Control:X0 Safety:X0  outputs:
Rearm  t=0.3  steps: Production:X0 Control:X0 Safety:X1  outputs:
Ack  t=0.5  steps: Production:X0 Control:X1 Safety:X1  outputs: Left Motor
3 s later  t=3.5  steps: Production:X0 Control:X2 Safety:X1  outputs:
Ack (Auto)  t=3.7  steps: Production:X0 Control:X3 Safety:X1  outputs: Heat Oven
Temperature = 85  t=3.8  steps: Production:X0 Control:X4 Safety:X1  outputs:
Start  t=4  steps: Production:X1 Control:X4 Safety:X1  outputs: Heat Oven Right Motor
Stop  t=4.2  steps: Production:X2 Control:X4 Safety:X1  outputs: Heat Oven Right Motor
3 s later  t=7.2  steps: Production:X0 Control:X4 Safety:X1  outputs:
```

| `GrafcetEngine` | Does |
|-----------------|------|
| `graphs(nets)` | the GRAFCETs to run together (cross references, forcing orders) |
| `start()`, `stop()`, `reset()` | compile the receptivities and start; stop and restore the initial steps |
| `setInput(name, value)`, `inputs()` | write and read the inputs (integers) |
| `step(dt)` | one cycle: time passes, forcing orders, evolution until stable, actions |
| `output(name)`, `outputs()` | the outputs |
| `firings()` | the transitions fired during the last cycle |
| `stepTime(net, id)`, `transitionTimer(net, id)` | how long a step is active; the delay of a transition |
| `receptivity(net, id)->timers()` | the state of the time operators (`30s/X1`) of a transition |
| `onWarning` | called with the warnings (unknown forcing target, unstable graphs) |

`DocumentInfo` (third argument of `importAllNetsFromJSON`) gives the descriptions and the kinds of the inputs and outputs.

## 3. Cycle time and earliest dates

```c++
#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/Flowshop.hpp"

Net net;
importFlowshop(net, "examples/Flowshops/simple.flowshop");
std::vector<std::vector<double>> dates;     // dates[t][k]
earliestDates(net, 3, dates);               // the Gantt chart: 3 cycles
CriticalCycleResult cc = findCriticalCycle(net);   // cc.durations: λ
std::cout << showDaterEquation(net, "", true, false).str();
```

This gives the numbers of [chapter 7](07-Timed-Event-Graphs.md#6-tutorial-how-fast-is-my-factory): `M1_C1: 0 6 12`, `λ = 6`, `M2_C1(k) = max(2 + M1_C1(k), M2(k - 1))`. `toSysLin()` and `toAdjacencyMatrices()` give the matrices, `operationOf()` splits `M1_C2` into its machine and its conveyor.

## 4. Simulate a timed net

`Engine` (`PetriNet/Engine.hpp`) runs a Petri net or a timed Petri net as a discrete event simulation: it owns the marking and does not modify the net. Time goes from event to event, so the dates are exact whatever the steps given to `advance()`; they are the earliest dates of the (max,+) dater equation for a timed event graph.

```c++
#include "PetriNet/Engine.hpp"

Engine::Options options;
options.policy = Engine::Policy::Random;   // or Priority, FirstDeclared
options.seed = 42;                         // 0: drawn, engine.seed() gives it
Engine engine(net, options);
engine.onFire = [](size_t t, double date, size_t k) { /* ... */ };
engine.reset();                            // marking of the net, date 0
engine.advanceTo(30.0);                    // or advance(dt), update(clock)
// engine.marking(), engine.flights() (tokens on their way),
// engine.firingDates()[t], engine.isDeadlocked()
```

`options.manual = true` fires a transition only when it is armed (`engine.arm(t)`), once, like the clicks of the editor on a Petri net. `Transition::delay` makes a transition wait, enabled, before firing; `Transition::priority` orders conflicts with `Policy::Priority`.

`Simulation` is the simulation of the editor on top of it: started and stopped, stepped at each frame, the marking shown in the places of the net and restored at the end, GRAFCET included.

```c++
#include "PetriNet/Simulation.hpp"

Simulation sim(net);
sim.onTrace.connect([](std::string const& s) { std::cout << s << "\n"; });
sim.start();
for (int i = 0; i < 300; ++i)
    sim.step(0.05f);              // 0.05 time unit per step
// sim.firingDates(transition_id): the dates of its firings
```

```text
t = 0.00  M1 starts P1 (k = 1), until 2.00
t = 2.00  M2 starts P1 (k = 1), until 3.00
t = 2.00  M1 starts P2 (k = 1), until 5.00
```

## 5. Draw a net in your window

`NetViewer` (`src/Editor/NetViewer.hpp`, sources in `TPNE_VIEWER_SOURCES`) draws a net, read only, inside the current Dear ImGui window of your program: the user can pan, zoom and fit it. Your program owns the net and its marking, so the viewer can show a GRAFCET run somewhere else.

```c++
tpne::NetViewer viewer;                 // keep it from frame to frame
tpne::NetViewer::Options options;       // captions, actions, highlighted arcs ...
viewer.draw("##grafcet", net, options); // inside ImGui::Begin() ... End()
```

## 6. Run your functions: the runtime

The runtime (`Runtime/Runtime.hpp`, `-lTimedPetriRuntime`) runs a net drawn in the editor by calling your functions: **a transition is a function**. When a transition can fire, the runtime takes its input tokens, calls its function, and when the function ends gives the output tokens, **colored by its result** (`Success`, `Failed` or a color of yours) unless the arc gives its own color. An arc from a place asks for a color: this is how the result of a function chooses what comes next.

```text
 (Ready) -> [detect] -> (Seen) -> [grasp] --Success--> (Held) -> [place] -> (Done)
                           ^                 \
                           +---- [retry] <----Failed
```

```c++
#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

Net net;                                         // drawn in the editor
loadFromFile(net, "pick.json");                  // arcs from Held: Success / Failed

Runtime rt(net, std::make_shared<ThreadPoolExecutor>(4));
rt.on("detect", [] { camera.detect(); });               // nothing: Success
rt.on("grasp",  [] { return gripper.grasp(); });        // bool: Success / Failed
rt.on("place",  [] { arm.place(); });
// "retry" has no function: it fires at once (rt.unregisteredTransitions())
rt.guard("place", [](Context const& c) { return !estop; });

auto order = rt.instantiate("order_42", {}, { { "Done", Success } });  // goal
Color result = rt.run(*order);       // blocking: Success, Deadlocked, Canceled, Failed
// or, without blocking, from the loop of your program (ImGui, ROS, Arduino):
while (rt.step(*order)) { /* ... */ }
```

- A function returns nothing (Success), a `bool` (Success or Failed) or a `Color`. Any type works with a free function `Color fire(T const&)`, and optionally `cancel()`, `pause()`, `resume()`, `isSynchronous()` (found by ADL).
- `InlineExecutor` (default) calls the functions in the thread of `run()` or `step()`; `ThreadPoolExecutor` on threads; `TaskflowExecutor` on [Taskflow](https://github.com/taskflow/taskflow) with `TPNE_WITH_TASKFLOW`.
- Only the thread of `run()` or `step()` touches the marking: the other threads post requests (end of a function, `inject()`, `cancel()`).
- A transition without input place fires when `rt.inject(*order, "button")` asks (a button, a message).
- Timed Petri nets: arcs with a duration give their tokens later; `Transition::delay` waits, enabled, before firing. A timeout is a timed transition in conflict with the slow one, going to a place of error.
- `order->log()` is the journal: each transition `Scheduled`, `Started`, `Completed` with its result and its date. `Options::journal` receives each line as it is written: save it with `toJsonLine()` (`Runtime/Journal.hpp`, JSON lines). After a crash, `rt.restore("order_42", journal, Runtime::Recovery::Restart)` rebuilds the instance from the journal and calls again the functions that were running (`Recovery::Fail` ends them with `Failed`).
- Without file (a board): build the net in the code, the positions are not needed: `Place& ready = net.addPlace(1u, "Ready"); Transition& detect = net.addTransition("detect"); net.addArc(ready, detect);` (and `net.findArc(...)->weight`, `->kind`, `->color = net.colors.add("Blue")`).
- `saveJournal(order->log(), net.colors, "orders.xes")` writes the journal as JSON lines, CSV or XES (process mining), by the extension.
- `JournalPublisher` (`Runtime/JournalPublisher.hpp`, `-lzmq`) publishes the journal: the editor shows the program running ([chapter 9](09-Remote-Control.md#4-follow-a-running-program)).
- A net can be the function of a transition of another net (a macro step): `parent.on("pick", SubNet(child_runtime, { { "Done", Success } }))` runs an instance of the child net at each firing; its end (`Success`, `Deadlocked`, `Canceled`, `Failed`) is the result of the transition, and canceling the parent cancels it.

## 7. Walk the structure of a net

Identifiers are indices: `net.places()[3].id == 3`, keys are `"P3"`, `"T0"`. An arc stores its two ends as `NodeId` (type and identifier), so a copied net never refers to the nodes of the original. The net gives the nodes and the arcs around a node:

```c++
for (Transition const& t : net.transitions())
{
    for (Arc const& arc : net.arcsIn(t))          // input places of t
        std::cout << net.placeOf(arc).key << " -> " << t.key << "\n";
    bool fireable = net.canFire(t);               // enabled and receptivity
}
Node const& from = net.node(arc.from);            // place or transition
```

Removing a node keeps identifiers `0 .. n-1`: the last node of the same type takes the identifier (and the key) of the removed one, and its arcs follow it. References to nodes and arcs are invalid after a removal. Algorithms give arcs by index in `net.arcs()` (`CriticalCycleResult::arcs`, the erroneous arcs of `isEventGraph()`, `NetViewer::Options::highlighted_arcs`).

---

## 8. Check the properties of a net

```c++
#include "Analysis/Analysis.hpp"            // -lTimedPetriAnalysis

Properties p = properties(net);             // explores up to 100000 markings
if (p.deadlock)                             // the marking and the path to it
    std::cout << "deadlock after " << p.deadlock_path.size() << " firings\n";
std::string error;
auto pinv = pInvariants(net, error);        // y.C = 0, y >= 0 (minimal supports)
std::cout << report(net);                   // all of it, as text
```

`reachabilityGraph()` and `coverabilityGraph()` (Karp-Miller) give the graphs of the markings, `toDot()` writes them for Graphviz. The durations are not considered; Karp-Miller refuses inhibitor and reset arcs and capacities. `tpne-check` is the same in a terminal or a CI: its exit code is 1 when a net can deadlock.

---

Next: [11. From the editor to the code](11-From-Editor-To-Code.md), the whole way with one example. The words are in the [glossary](Glossary.md); books and other tools in the [bibliography](Bibliography.md).
