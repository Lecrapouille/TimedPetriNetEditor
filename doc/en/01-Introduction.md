# 🧩 1. Introduction


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/01-Introduction.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/01-Introduction.md)

**What you will learn**

- what a discrete event system is, and why we draw it with a Petri net;
- the families of nets of the editor (Petri nets, extended Petri nets, timed Petri nets, timed event graphs, GRAFCET) and the question each one answers;
- what the editor offers for each family, and where the documentation explains it;
- the story of the project, from a (max,+) toolbox to a runtime.

**Prerequisites**: none. The words in **bold** are in the [glossary](Glossary.md).

---

## 1. 🏭 Discrete event systems

Look at a coffee machine. It waits for a coin; a coin falls; it checks the coin; it heats water; it pours a coffee; it waits again. Its state does not change continuously like the temperature of a room: it jumps from one situation to another when something happens. Each of these "somethings" is an **event**, and a system whose state changes only at events is a **discrete event system**.

Most of the systems engineers build are of this kind:

| System | Its events | What we want to know |
|--------|------------|----------------------|
| a factory | a part enters a machine, a machine ends its job | how many parts per hour? which machine slows the others? |
| a crossroads | a light turns green, a car passes | can two flows of cars cross at the same time? |
| a call center | a call arrives, an operator answers, hangs up | who waits, and how long? |
| a robot | an object is seen, grasped, dropped | what if the gripper misses? |
| a computer program | a message arrives, a task ends, a lock is taken | can it block for ever (a deadlock)? |
| a railway network | a train leaves, arrives | what timetable can it keep? |

Four phenomena make these systems hard to understand, and they appear in every one of them:

- **Concurrency**: several things happen at the same time and independently (two machines work in parallel).
- **Synchronization**: a thing waits for several others (an assembly waits for all its parts; a coffee is poured only when the water is hot *and* the coin is accepted).
- **Choice** (conflict): one situation can lead to several futures (the coin is good or fake; the caller needs advice or an ambulance).
- **Shared resources**: several actors use something that exists in a limited number (operators, forks, a robot arm, a memory buffer).

A model of a discrete event system must show these four things clearly, and let us compute answers to the questions of the table.

## 2. 🔵 Why Petri nets

Why not a state machine (an automaton), the drawing everybody knows, with circles for states and arrows for events? Because an automaton has **one** current state. Two machines of 3 states each, working in parallel, need an automaton of 3 × 3 = 9 states; ten machines need 3¹⁰ = 59049 states. This is the **state explosion**: the drawing grows exponentially with the number of parts of the system, and nobody can read it any more.

In 1962, in his thesis *Kommunikation mit Automaten*, **Carl Adam Petri** proposed another drawing, where the state is **distributed**:

```text
   place          transition        arc            token
   (a condition,  (an event)        (who feeds     (a resource, a part,
    a stock)                         whom)          "this is true")
     ( )             ▭               ──►              ●
```

- a **place** (circle) is a local condition or a stock: "the machine is free", "parts waiting";
- a **token** (dot) in a place says that the condition holds, or counts the items of the stock;
- a **transition** (bar) is an event; it can happen when each place before it holds a token, and when it happens it **fires**: it takes one token from each place before it and puts one token in each place after it.

The state of the whole system, the **marking**, is the number of tokens in each place. Ten machines of 3 states are now ten small loops of 3 places, 30 places in all, side by side: the drawing grows like the system, not like the number of its states. And the four phenomena have a simple shape:

```text
 concurrency        synchronization      choice              shared resource
   ┌─► ▭ ─►            ─► ( ) ─┐           ┌─► ▭ ─►            ▭ ◄── (●) ──► ▭
  ( )                          ▭ ─►       (●)                   │ mutex ▲      │
   └─► ▭ ─►            ─► ( ) ─┘           └─► ▭ ─►            └───────┴──────┘
 two events that     an event that       two events that     two users of one
 can happen in       waits for two       want the same       token, given back
 any order           conditions          token               after use
```

A Petri net is both a **drawing** that people can discuss and a **mathematical object**: two matrices (what each transition takes and gives) and a vector (the initial marking). From them, algorithms answer the questions of the table: can the net block? can a place grow without limit? which markings can be reached? With time added, how fast does it go? These are the subjects of the next chapters.

![A coffee machine as a Petri net](pics/03-Editor-Coffee.png)

*A coffee machine as a Petri net in the editor ([examples/Petri-Nets/Coffee](../../examples/Petri-Nets/Coffee/README.md)): the green transitions can fire.*

## 3. 🧭 The families of nets of the editor

Petri nets have many variants. The editor offers the ones that answer the questions of engineers, each one adding something to the previous:

```text
                        ┌──────────────── extended Petri nets ────────────────┐
                        │  weights, capacities, inhibitor and reset arcs,     │
                        │  priorities, colored tokens                         │
                        │   ┌──────────────── Petri nets ─────────────────┐   │
                        │   │  places, transitions, tokens, firing rule   │   │
                        │   │   ┌───────── event graphs ──────────┐       │   │
                        │   │   │ each place: one transition in,  │       │   │
                        │   │   │ one transition out: no choice   │       │   │
                        │   │   └─────────────────────────────────┘       │   │
                        │   └─────────────────────────────────────────────┘   │
                        └─────────────────────────────────────────────────────┘
   + time (durations, delays)   →  timed Petri nets, timed event graphs + (max,+)
   + inputs, outputs, conditions →  GRAFCET (one token per step at most), GEMMA
   + functions of a program      →  the runtime
```

| Family | It adds | The question it answers | Chapters |
|--------|---------|-------------------------|----------|
| **Petri net** | the firing rule | Which situations can happen? Can it block? Is a resource never used twice? | [3](03-Petri-Nets.md), [4](04-Analysis.md) |
| **Extended Petri net** | weights, capacities, **inhibitor arcs** (fire only if a place is empty), **reset arcs** (empty a place), priorities, **colored tokens** | The same, for systems that need to count, to test for zero, to clear a stock, to choose by a result | [5](05-Extended-Petri-Nets.md) |
| **Timed Petri net** | **durations** on arcs, **delays** on transitions | When does each event happen? What does a timeout do? | [6](06-Timed-Petri-Nets.md) |
| **Timed event graph** | a timed net without choice | How fast can it go (the **cycle time**)? What is the bottleneck (the **critical circuit**)? What is the schedule (Gantt chart)? Answered exactly by the **(max,+) algebra** | [7](07-Timed-Event-Graphs.md), [8](08-Max-Plus-Algebra.md) |
| **GRAFCET** | steps with **actions**, transitions with **receptivities** on inputs and time (IEC 60848) | How does the controller of a machine react to its buttons and sensors? | [9](09-GRAFCET.md) |
| **GEMMA** | several GRAFCETs that force each other | How does a machine start, stop and recover after a failure? | [10](10-GEMMA.md) |
| **Runtime** | a function of your program per transition | How do I make my program follow the net I drew? | [11](11-Runtime.md), [12](12-Code-Generation.md), [13](13-Debugging.md) |

You choose the kind of a net in the toolbar of the editor (or with *Edit > Type of net*); the extensions are given to the arcs and places of a Petri net or a timed Petri net.

## 4. 🛠️ What the editor offers

| | For every family | Petri nets and extensions | Timed nets | Timed event graphs | GRAFCET, GEMMA | Programs |
|-|-|-|-|-|-|-|
| **Draw** | the mouse only: the left button draws, the right one moves; undo, copy, paste, layouts | weights, kinds of arcs, capacities, colors with a right click | durations, delays | the compact drawing (places on the arcs), the canonical form | receptivities checked while typing, actions, forcing orders | |
| **Run** | tokens moving on the net, speed x0.1 to x50, a journal of the firings | click on a transition to fire it | the time runs, conflicts by priority or at random | the same, compared to the theory | a control panel with buttons, selectors, sensors; outputs; a timing diagram | the runtime calls your functions |
| **Analyze** | | deadlocks and the path to them, bounds, liveness, P- and T-invariants, `tpne-check` for a CI | plot of the firing dates | cycle time, critical circuit, Gantt chart, (max,+) equations and matrices | | |
| **Export** | JSON, PNML, GRML, SVG, LaTeX, Graphviz, draw.io ... | | | Julia (MaxPlus.jl), `.teg` | C++ code for an Arduino | the skeleton of a C++ program, journals for process mining |
| **Debug** | | | | | the generated code followed live | the editor follows your running program |

Three views of the editor, made by the editor itself:

![A call center running](pics/01-Editor-EmergencyCalls.png)

*An emergency call center (a Petri net with colored tokens, [examples/Petri-Nets/EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md)).*

![A flowshop and its Gantt chart](pics/01-Editor-Flowshop.png)

*A factory as a timed event graph: its critical circuit in red, its schedule as a Gantt chart ([chapter 7](07-Timed-Event-Graphs.md)).*

![A dryer driven by a GEMMA](pics/01-Editor-GEMMA.png)

*A dryer driven by three GRAFCETs (a GEMMA): the control panel and the outputs on the right and at the bottom ([chapter 10](10-GEMMA.md)).*

## 5. 📜 The story of the project

**The (max,+) algebra.** In the 1980s, at INRIA, a group of researchers (Guy Cohen, Didier Dubois, Jean-Pierre Quadrat, Michel Viot, then the *Max-Plus* working group) found that the dates of the events of a timed event graph follow *linear* equations, provided one replaces the addition by `max` and the multiplication by `+`. The schedule of a factory or of a railway network became a problem of linear algebra; its speed became an eigenvalue. The book *Synchronization and Linearity* (Baccelli, Cohen, Olsder, Quadrat, 1992) gathers this theory, and a (max,+) toolbox for Scilab, later ScicosLab, computed it, with the Howard algorithm for the eigenvalue.

**MaxPlus.jl.** ScicosLab is no longer developed. The author of this editor, the son of Jean-Pierre Quadrat, ported the toolbox of his father to Julia: [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl). One thing was missing in both tools: you had to type the matrices by hand.

**A timed event graph editor (2021).** This editor started in June 2021 as the missing graphical half of the toolbox: draw the net with the mouse, and the matrices, the equations, the cycle time and the critical circuit come by themselves. The Howard algorithm of ScicosLab was brought in (July 2021), the Julia export, then the flowshops (a factory described as a table of times, turned into a timed event graph) and their Gantt charts. The Petri net editors found on GitHub did not help: most were abandoned, needed Windows, Java or C#, or could not be extended.

**GRAFCET (2021 – 2026).** The author is an engineer, and engineers in automation draw GRAFCETs. Two weeks after the first commit, after watching a video in which a GRAFCET is coded by hand in C for an Arduino, the editor already generated the C++ code of a GRAFCET. Years later, this became a complete GRAFCET of the standard IEC 60848 (edges, timers, all the action qualifiers, forcing orders between graphs), a GEMMA with several graphs, a control panel with real buttons and a timing diagram, and C++ code checked against the engine of the editor.

**Extended Petri nets and the runtime (2026).** Petri nets then got their classic extensions: weights, capacities, inhibitor and reset arcs, priorities, colored tokens, and the analysis of their properties (deadlocks, bounds, liveness, invariants). Last came the idea of [Symmetri](https://github.com/thorstink/Symmetri): a net drawn in the editor runs a program, each transition calling a function, the result of the function choosing what comes next. The editor became the IDE of this runtime: it writes the skeleton of the program, and it follows the program while it runs.

The projects using it today include [RobotIK](https://github.com/Lecrapouille/Robotik), which drives simulated robot cells with the GRAFCET engine and the (max,+) algorithms.

## 6. 📖 How to read this documentation

- Each learning chapter goes **theory first** (an intuition, a definition, a worked example with every number), then **in the editor** (how the editor shows and solves it), then **try it** (the examples to open).
- The examples are in [`examples/`](../../examples/README.md), one folder per example, grouped by family; each has a picture, a README and often a description written on the canvas above the net.
- Menus are written *File > Open*, keys **Space**, files [like this](../../examples/README.md).
- Every output and picture comes from a real run: you can reproduce all of them (`tools/screenshots.sh` makes the pictures again).

---

Next: [2. Getting started](02-Getting-Started.md) — build the editor and draw your first net.
