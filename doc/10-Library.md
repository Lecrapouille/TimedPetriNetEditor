# 10. Use the library in C++

What you will find here: how a program uses the nets without the editor:
load a file, run a GRAFCET, compute the dates of a timed event graph,
simulate it, draw a net in your own Dear ImGui window.

The editor is made of two libraries: `libTimedPetriNet` (nets, simulation,
GRAFCET engine, (max,+) algorithms, imports, exports; no graphics) and
`libTimedPetriGUI` (the editor). [RobotIK](https://github.com/Lecrapouille/Robotik)
drives its robot cells with the first one.

---

## 1. Build against the library

After `make`, the libraries are in `build/`. A program `main.cpp`:

```sh
g++ -std=c++20 -Isrc -Iexternal/json/include -Iexternal main.cpp \
    -Lbuild -lTimedPetriNet -Wl,-rpath,$PWD/build -o main
```

A project built with Make can compile the sources instead: define
`TPNE_DIR` and include `TimedPetriNet.mk`, which gives `TPNE_SOURCES`,
`TPNE_INCLUDES` and `TPNE_CXXFLAGS` (and `TPNE_VIEWER_SOURCES` for §5).

## 2. Run a GRAFCET

The dryer of [chapter 4](04-GRAFCET-and-GEMMA.md#3-tutorial-the-gemma-of-a-dryer),
without the editor. The program writes the inputs, calls `step(dt)` once per
cycle, and reads the steps and the outputs:

```c++
#include "PetriNet/GrafcetEngine.hpp"
#include "PetriNet/Imports/Imports.hpp"
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

`DocumentInfo` (third argument of `importAllNetsFromJSON`) gives the
descriptions and the kinds of the inputs and outputs.

## 3. Cycle time and earliest dates

```c++
#include "PetriNet/Algorithms.hpp"
#include "PetriNet/Flowshop.hpp"

Net net;
importFlowshop(net, "data/examples/simple.flowshop");
std::vector<std::vector<double>> dates;     // dates[t][k]
earliestDates(net, 3, dates);               // the Gantt chart: 3 cycles
CriticalCycleResult cc = findCriticalCycle(net);   // cc.durations: λ
std::cout << showDaterEquation(net, "", true, false).str();
```

This gives the numbers of [chapter 3](03-Timed-Nets-and-Max-Plus.md#3-tutorial-how-fast-is-my-factory):
`M1_P1: 0 6 12`, `λ = 6`, `M2_P1(k) = max(2 + M1_P1(k), M2(k - 1))`.
`toSysLin()` and `toAdjacencyMatrices()` give the matrices, `operationOf()`
splits `M1_P2` into its machine and its piece.

## 4. Simulate a timed net

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
t = 0.05  M1 starts P1 (k = 1), until 2.05
t = 2.10  M1 starts P2 (k = 1), until 5.10
t = 2.10  M2 starts P1 (k = 1), until 3.10
```

## 5. Draw a net in your window

`NetViewer` (`src/Editor/NetViewer.hpp`, sources in `TPNE_VIEWER_SOURCES`)
draws a net, read only, inside the current Dear ImGui window of your
program: the user can pan, zoom and fit it. Your program owns the net and
its marking, so the viewer can show a GRAFCET run somewhere else.

```c++
tpne::NetViewer viewer;                 // keep it from frame to frame
tpne::NetViewer::Options options;       // captions, actions, highlighted arcs ...
viewer.draw("##grafcet", net, options); // inside ImGui::Begin() ... End()
```

## 6. Walk the structure of a net

Identifiers are indices: `net.places()[3].id == 3`, keys are `"P3"`, `"T0"`.
An arc stores its two ends as `NodeId` (type and identifier), so a copied
net never refers to the nodes of the original. The net gives the nodes and
the arcs around a node:

```c++
for (Transition const& t : net.transitions())
{
    for (Arc const& arc : net.arcsIn(t))          // input places of t
        std::cout << net.placeOf(arc).key << " -> " << t.key << "\n";
    bool fireable = net.canFire(t);               // enabled and receptivity
}
Node const& from = net.node(arc.from);            // place or transition
```

Removing a node keeps identifiers `0 .. n-1`: the last node of the same
type takes the identifier (and the key) of the removed one, and its arcs
follow it. References to nodes and arcs are invalid after a removal.
Algorithms give arcs by index in `net.arcs()` (`CriticalCycleResult::arcs`,
the erroneous arcs of `isEventGraph()`, `NetViewer::Options::highlighted_arcs`).

---

That is the end of the reference. The words are in the
[glossary](Glossary.md); books and other tools in the
[bibliography](Bibliography.md).
