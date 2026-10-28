# 3. 🔵 Petri nets


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/03-Petri-Nets.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/03-Petri-Nets.md)

**What you will learn**

- what a Petri net is, in pictures and in its mathematical form;
- the firing rule, computed by hand on real examples, and what a marking and a firing sequence are;
- the difference between **concurrency** (things happen independently) and **conflict** (things compete);
- the patterns you will use to model real systems: sequence, fork and join, choice, mutual exclusion, shared resources, buffers, loops;
- the subclasses of Petri nets (state machines, event graphs, free choice nets) and why they matter;
- how the editor draws a net, runs it by hand, and solves conflicts.

**Prerequisites**: [chapter 1](01-Introduction.md) (what discrete event systems are) and [chapter 2](02-Getting-Started.md) (build the editor, draw with the mouse).

---

## 1. Why Petri nets?

A **discrete event system** is a system whose state changes by jumps, at instants called **events**: a coin falls in a coffee machine, a robot grasps a part, a phone rings, a light turns green. Between two events nothing changes. Such systems are everywhere in factories, computers, networks, transport and organizations, and they have three features that make them hard to design:

- **parallelism**: several things happen at the same time (two robots work, four operators answer the phone);
- **synchronization**: something waits for several others (the coffee is served only when the water is hot *and* the coin is accepted);
- **sharing**: several activities compete for the same resources (one fork for three philosophers, one tool for two robots).

A state machine (an automaton) can describe them, but its number of states explodes: two independent machines of 10 states need 10 × 10 = 100 states together. In 1962, Carl Adam **Petri** proposed in his thesis a graphical and mathematical model where the state is *distributed*: each part of the system keeps its own local state, and the parts interact only where they meet. This model, the **Petri net**, stays small when the system grows, shows the parallelism on the drawing, and can be analyzed by algorithms ([chapter 4](04-Analysis.md)).

## 2. The elements of a Petri net

A Petri net is a graph with two kinds of nodes and arcs between them:

| Element | Meaning | Symbol | Detail |
|---------|---------|--------|--------|
| **Place** | A condition, a stock | `( )` | Drawn as a circle; local state or stock ("the machine is idle", "parts in the buffer", "the door is open"). |
| **Transition** | An event | `▭` | Drawn as a bar or rectangle; an action ("start", "send", "the door opens"). |
| **Arc** | Who feeds whom | `──►` | An arrow from a place to a transition (**input place**) or from a transition to a place (**output place**). Never between two places nor between two transitions: a Petri net is a **bipartite graph**. |
| **Token** | A resource, a part, "true" | `●` | Held in places (black dots, or a number when there are many): resources, parts, people, or simply that a condition is true. |

The number of tokens in every place is the **marking** of the net: it is the state of the whole system. The marking at the start is the **initial marking**, written `M0`.

Here is the smallest net, the example [FirstNet](../../examples/Petri-Nets/FirstNet/README.md):

```text
   Ready          Fire          Done
    (●)  ──────►   ▭   ──────►  ( )
```

![First net](../../examples/Petri-Nets/FirstNet/FirstNet.png)

*The net drawn by the editor. The description written in the file is shown above the net: each example tells you what to try.*

## 3. The firing rule

The dynamics of a Petri net is one single rule.

> A transition is **enabled** when each of its input places holds at least one token. An enabled transition can **fire**: it removes one token from each input place and adds one token to each output place. Firing is instantaneous.

In [FirstNet](../../examples/Petri-Nets/FirstNet/README.md), `Fire` is enabled because `Ready` holds a token. After the firing, `Ready` is empty and `Done` holds a token; `Fire` is no longer enabled and nothing can happen any more.

| Before | After |
|--------|-------|
| `(●) ──► ▭ ──► ( )` | `( ) ──► ▭ ──► (●)` |

Three remarks that surprise beginners:

1. Tokens are not "moved" along a path: they are **consumed** and **produced**. A transition with one input and two outputs creates a token; with two inputs and one output it destroys one. The number of tokens is not constant in general.
2. An enabled transition *can* fire; it does not have to. A Petri net says what is **possible**, not what happens first. Time and priorities come in [chapter 6](06-Timed-Petri-Nets.md) and [chapter 5](05-Extended-Petri-Nets.md).
3. A transition with no input place is always enabled: it is a **source** (the arrival of clients, of cars). A transition with no output place is a **sink** (clients leave).

### 3.1 A worked example: the producer and the consumer

The example [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) has five places and four transitions:

```text
  Produce          Send             Accept          Consume
    ▭ ──► (●) ──► ▭ ──► (●●●) ──► ▭ ──► ( ) ──► ▭
    ▲   Producer    │     Buffer    ▲   Accepted   │
    │    ready      ▼               │              ▼
    └────────── (●) Idle            └──── (●●) Ready
```

![Producer consumer](../../examples/Petri-Nets/ProducerConsumer/ProducerConsumer.png)

Number the places and the transitions as the editor does (`P0` ... `P4`, `T0` ... `T3`, see §8) and write the marking as a vector, in the order of the places:

| Place | `P0` Producer ready | `P1` Idle | `P2` Buffer | `P3` Accepted | `P4` Ready |
|-------|:---:|:---:|:---:|:---:|:---:|
| `M0`  | 1 | 1 | 3 | 0 | 2 |

Which transitions are enabled in `M0`?

- `T0` Send needs `P0` ≥ 1: yes.
- `T1` Produce needs `P1` ≥ 1: yes.
- `T2` Accept needs `P2` ≥ 1 and `P4` ≥ 1: yes.
- `T3` Consume needs `P3` ≥ 1: no.

Fire `Send`: remove a token from `P0`, add one to `P1` and one to `P2`. Then fire `Accept`: remove one from `P2` and one from `P4`, add one to `P3`. Then `Consume`. The library computes the same (a small program calling `fire()` of `PetriNet/Firing.hpp`, [chapter 16](16-Library.md)):

| Step | Marking | Enabled transitions |
|------|---------|---------------------|
| `M0` | `(1, 1, 3, 0, 2)` | Send, Produce, Accept |
| fire Send | `(0, 2, 4, 0, 2)` | Produce, Accept |
| fire Accept | `(0, 2, 3, 1, 1)` | Produce, Accept, Consume |
| fire Consume | `(0, 2, 3, 0, 2)` | Produce, Accept |

The list of transitions fired, `σ = Send Accept Consume`, is a **firing sequence** (the order matters). A marking that you can reach from `M0` by firing transitions one after another is a **reachable marking**. The shorthand `M0 [σ> M` means: start at marking `M0`, fire each transition of `σ` in order, and you end at marking `M`. Here, `M0 [Send Accept Consume> (0, 2, 3, 0, 2)`.

## 4. The mathematical definition

Drawings are for humans; algorithms need numbers. A Petri net is a tuple

```text
N = (P, T, Pre, Post, M0)
```

- `P = {p1, ..., pn}` the finite set of places, `T = {t1, ..., tm}` the finite set of transitions, `P ∩ T = ∅`;
- `Pre` the **input matrix** (`n` rows, `m` columns): `Pre(p, t)` is the number of tokens that `t` takes from `p` (the weight of the arc `p ─► t`, 0 when there is no arc);
- `Post` the **output matrix**: `Post(p, t)` is the number of tokens that `t` gives to `p` (the weight of the arc `t ─► p`);
- `M0` the initial marking, a vector of `n` natural numbers.

In an ordinary net all the weights are 1; weights greater than 1 are an extension ([chapter 5](05-Extended-Petri-Nets.md#1-weights)). With this notation the firing rule is two lines:

| Rule | Text | | Formula |
|------|:----:|-|---------|
| Enable | `t` is enabled in `M` | ⇔ | `M(p) ≥ Pre(p, t)` for every place `p` |
| Fire   | firing `t` gives `M'` | = | `M' = M − Pre(·, t) + Post(·, t)` |

For the producer and the consumer:

**Pre:**

| Place | T0 | T1 | T2 | T3 |
|-------|:--:|:--:|:--:|:--:|
| P0 | 1 | 0 | 0 | 0 |
| P1 | 0 | 1 | 0 | 0 |
| P2 | 0 | 0 | 1 | 0 |
| P3 | 0 | 0 | 0 | 1 |
| P4 | 0 | 0 | 1 | 0 |

**Post:**

| Place | T0 | T1 | T2 | T3 |
|-------|:--:|:--:|:--:|:--:|
| P0 | 0 | 1 | 0 | 0 |
| P1 | 1 | 0 | 0 | 0 |
| P2 | 1 | 0 | 0 | 0 |
| P3 | 0 | 0 | 1 | 0 |
| P4 | 0 | 0 | 0 | 1 |

`M0 − Pre(·, T0) + Post(·, T0) = (1,1,3,0,2) − (1,0,0,0,0) + (0,1,1,0,0) = (0,2,4,0,2)`: the marking computed above. The difference `C = Post − Pre` is the **incidence matrix**; [chapter 4](04-Analysis.md#5-the-incidence-matrix-and-the-state-equation) uses it to prove properties without simulating.

## 5. Concurrency and conflict

Two enabled transitions can be in two very different situations.

### 5.1 Concurrency: independent events

In [ForkJoin](../../examples/Petri-Nets/ForkJoin/README.md), `Fork` puts a token in each branch. `Task A` and `Task B` are then both enabled and do not share any input place: firing one does not disable the other. They are **concurrent**: they can happen in any order, or at the same time.

![Fork and join](../../examples/Petri-Nets/ForkJoin/ForkJoin.png)

| Step | Marking | Enabled transitions |
|------|---------|---------------------|
| `M0` | `(1, 0, 0, 0, 0, 0)` | Fork |
| fire Fork | `(0, 1, 1, 0, 0, 0)` | Task A, Task B |
| fire Task B | `(0, 1, 0, 0, 1, 0)` | Task A |
| fire Task A | `(0, 0, 0, 1, 1, 0)` | Join |
| fire Join | `(0, 0, 0, 0, 0, 1)` | Restart |

`Join` waits for both branches: this is a **synchronization** (a rendez-vous). Concurrency is the strength of Petri nets: two branches of 10 steps each are 20 places, not 100 states.

### 5.2 Conflict: a choice

In [Conflict](../../examples/Petri-Nets/Conflict/README.md), one coin and two transitions `Tea` and `Coffee` share the input place `Coin`. Both are enabled, but firing one **disables** the other: they are in **conflict** (a **choice**).

![Conflict](../../examples/Petri-Nets/Conflict/Conflict.png)

| Step | Marking | Enabled transitions |
|------|---------|---------------------|
| `M0` | `(1, 0, 0)` | Tea, Coffee |
| fire Tea | `(0, 1, 0)` | *(none — Coffee not enabled)* |

A Petri net does **not** say which transition fires: the model is **non-deterministic**. This is a feature when you describe what *may* happen (a client may want tea or coffee, a part may be good or bad), and a danger when you describe what a controller *must* do. Who decides?

- **in the editor, for a Petri net**: you do. Nothing fires by itself; you click on the transition you want (§9).
- **in the editor, for a timed Petri net**: transitions fire by themselves, and *Simulation > Conflicts* chooses: **Random** (with a **seed**, written in the console, to replay the same simulation), **Highest priority** (the `priority` of the transitions, [chapter 5](05-Extended-Petri-Nets.md#5-priorities)) or **First declared** (the lowest identifier).
- **in a GRAFCET**, the receptivities of the transitions decide, and the designer makes them exclusive ([chapter 9](09-GRAFCET.md)).
- **in a program driven by the runtime**, the result of a function colors the token and the color chooses ([chapter 11](11-Runtime.md)).

The old documentation of the editor said it well: when you design a real system, a conflict that nobody resolves is a bug waiting to happen.

## 6. Patterns of modeling

Real nets are built from a few patterns. Learn to recognize them; every example of [`examples/Petri-Nets`](../../examples/README.md) is a combination of them.

| Pattern | Drawing | Meaning | Example |
|---------|---------|---------|---------|
| **Sequence** | <pre>(●) ─► ▭ ─► ( )&#10;  ( ) ◄─ ▭ ◄─┘</pre> | one step after the other | [FirstNet](../../examples/Petri-Nets/FirstNet/README.md) |
| **Fork** (AND split) | <pre>(●) ─► ▭ ─► ( )&#10;       └───► ( )</pre> | start several activities in parallel | `Fork` of [ForkJoin](../../examples/Petri-Nets/ForkJoin/README.md) |
| **Join** (AND join) | <pre>(●) ─┐&#10;     ├─► ▭ ─► (●)&#10;(●) ─┘</pre> | wait for all of them | `Join` of ForkJoin, `Dispense` of [Coffee](../../examples/Petri-Nets/Coffee/README.md) |
| **Choice** (OR split) | <pre>(●) ─┬─► ▭&#10;     └─► ▭</pre> | one of them will happen | [Conflict](../../examples/Petri-Nets/Conflict/README.md), `Accept` / `Reject Coin` of Coffee |
| **Merge** (OR join) | <pre>▭ ─┐&#10;    ├─► (●)&#10;▭ ─┘</pre> | several ways lead to the same state | `Level 1 operators` of [EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md) |
| **Loop** | <pre>↻ (●)</pre> | a cyclic behavior | `Restart` of ForkJoin |
| **Resource** | <pre>▭ ─► (●) ─► ▭</pre> | a machine, a tool, an operator | `Tool 1` of [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) |
| **Mutual exclusion** | <pre>▭ ─► (●) ◄─ ▭</pre> | one at a time | `Mutex` of [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) |
| **Buffer** | <pre>▭ ─► (●●) ─► ▭</pre> | decouple two processes | `Buffer` of ProducerConsumer |
| **Read arc** (test) | <pre>(●) ⇄ ▭</pre> | the transition needs the condition but does not change it | `Start` reads `Door shut` in [ElevatorControl](../../examples/Petri-Nets/ElevatorControl/README.md) |
| **Counter** | <pre>(●●●)</pre> | at most N at the same time | `Remaining` of [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) |

### 6.1 Mutual exclusion

Two processes, each one `Idle` or in its `Critical` section, share the token of `Mutex`. `Enter A` takes it, `Leave A` gives it back. While A is in its critical section, `Enter B` is not enabled: it misses the `Mutex` token.

![Mutual exclusion](../../examples/Petri-Nets/MutualExclusion/MutualExclusion.png)

### 6.2 Shared resources and the dining philosophers

The philosophers problem (Dijkstra, 1965) is the classic of shared resources. In [Philosophers](../../examples/Petri-Nets/Philosophers/README.md), three philosophers share one fork and two knives; each one needs two pieces of cutlery to eat. Each piece is a resource place with one token. Who can eat at the same time? Phi0 (fork + knife 1) and Phi1 (fork + knife 2) never together, since they share the fork; Phi2 (both knives) never with anybody.

![Philosophers](../../examples/Petri-Nets/Philosophers/Philosophers.png)

When resources are taken **one at a time** and in different orders, a **deadlock** can appear: in [Deadlock](../../examples/Petri-Nets/Deadlock/README.md), robot A takes tool 1 then tool 2, robot B takes tool 2 then tool 1. After `A takes 1` and `B takes 2`, each robot waits for the tool held by the other, for ever. [Chapter 4](04-Analysis.md#3-properties) shows how the editor finds such deadlocks for you.

### 6.3 A real-life net: the emergency call center

[EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md) combines all the patterns. Seven victims call; four level 1 operators (a resource) answer and screen the calls (a choice in three): advice they give themselves, urgent calls they transfer to a level 2 operator and hang up, critical calls they keep until a level 2 operator joins them in a three-way call (a join of two resources). See this [Youtube video](https://www.youtube.com/watch?v=yP5qajIL_YM&t=261s) (in French).

![Emergency calls](../../examples/Petri-Nets/EmergencyCalls/EmergencyCalls.png)

Play it in the editor: you will see the calls pile up in `Hold music` when the four level 2 operators are busy, and the level 1 operators blocked in three-way calls. Its numbers are written on the arcs as durations, but this net is a Petri net: the durations are kept in the file and ignored (they are used if you change its type to a timed Petri net, [chapter 6](06-Timed-Petri-Nets.md)).

## 7. Subclasses of Petri nets

Restricting the structure of a net gives subclasses with stronger properties and faster algorithms. Three of them matter here:

| Subclass | Structure | What it can describe | What it cannot |
|----------|-----------|----------------------|----------------|
| **State machine** | each transition has exactly one input and one output place | choices; with one token, a classic automaton | parallelism, synchronization |
| **Event graph** (marked graph) | each place has exactly one input and one output transition | parallelism, synchronization, good mathematical properties | choices, conflicts |
| **Free choice net** | when two transitions share an input place, they have the same input places | both, with choices that do not depend on the rest of the net | choices mixed with synchronization |

**Event graphs** are the subclass of the second half of this documentation: since a token in a place can only go to one transition, there is no conflict, and with durations the dates of all the events follow linear equations in the (max,+) algebra ([chapters 7](07-Timed-Event-Graphs.md) and [8](08-Max-Plus-Algebra.md)). The editor checks this structure before computing a cycle time, and says which places break it:

```text
The Petri net is not an event graph. Because:
  P0 has more than one input arc: T3 T4
```

The editor has no special mode for state machines: they are Petri nets, and for large reactive systems, [Harel's statecharts](https://www.itemis.com/en/products/itemis-create/documentation/user-guide/overview_what_are_state_machines) or [SyncCharts](http://www-sop.inria.fr/members/Charles.Andre/CA%20Publis/Cesa96/SyncCharts_Cesa96.pdf) are better suited. You can you my Python tool converting PlantUML statecharts to C++ (work in progress).

## 8. Identifiers and captions

The editor names the nodes itself: places `P0, P1, ...`, transitions `T0, T1, ...`. These **identifiers** are consecutive integers without holes, so that they are directly the row and column indices of the matrices `Pre`, `Post`, `C` and of the (max,+) matrices: no lookup table is needed. When you delete a node, the last node of the same kind takes its identifier (and its arcs follow it). The text you write on a node is its **caption**; several nodes may have the same caption (the three `End of call` of EmergencyCalls). Arcs have no identifier: the pair (origin, destination) is unique.

## 9. In the editor

### 9.1 Draw

*File > New > Petri net* (or choose *Petri net* in the toolbar). There is no mode to choose: **the left button draws, the right button moves**.

| You do | You get |
|--------|---------|
| **left click** in the void | a place |
| **right click** in the void | a transition |
| **left drag** from a node to a node | an arc (place ─► transition or transition ─► place) |
| **left drag** from a node to the void | a new node of the other kind, with its arc |
| **double click** on a node | edit its caption |
| mouse on a place, **+** / **-** | add or remove a token |
| **right click** on a place | its menu: *Tokens*, *Max tokens* (capacity, [chapter 5](05-Extended-Petri-Nets.md#2-capacities)) |
| **right click** on an arc | its menu: weight, kind, color of the tokens ([chapter 5](05-Extended-Petri-Nets.md)) |
| **Delete** key | delete the selection, or what is under the mouse |

The status bar at the bottom always says what each button would do. Every gesture is in the [editor reference](14-Editor-Reference.md).

![The editor with Coffee](pics/03-Editor-Coffee.png)

*The coffee machine in the editor. Enabled transitions are green. The inspector on the right lists the places with their tokens.*

### 9.2 Run by hand

Press **Space**: the simulation starts, the frame of the net turns green and the console says what you can do. In a Petri net, nothing fires by itself:

- **left click** on a green (enabled) transition fires it;
- **+** / **-** on a place change its tokens while it runs;
- **Space** again stops the simulation and **gives back the initial marking**: you can try as many scenarios as you want.

![Simulation of the emergency calls](pics/03-Simulation-EmergencyCalls.png)

*The emergency calls running: the frame is green, `Level 1 answers` is the only enabled transition at the start. The console gives the seed of the random choices.*

Each firing is written in the *Journal* (next to the console), with its number k: the 3rd firing of `Accept` is `k = 3`. The journal can be saved and replayed ([chapter 13](13-Debugging.md)).

### 9.3 Check

*Analysis > Properties* tells whether the net can deadlock, whether a place can grow for ever, and which sums of tokens never change: this is [chapter 4](04-Analysis.md).

## 10. Try it

| Example | What to do | What to observe |
|---------|------------|-----------------|
| [FirstNet](../../examples/Petri-Nets/FirstNet/README.md) | Space, click `Fire` | the firing rule; the end of all activity |
| [ForkJoin](../../examples/Petri-Nets/ForkJoin/README.md) | fire the tasks in both orders | concurrency and synchronization |
| [Conflict](../../examples/Petri-Nets/Conflict/README.md) | choose tea, stop, choose coffee | a choice disables the other branch |
| [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) | fire `Send` many times | the buffer grows without limit |
| [Coffee](../../examples/Petri-Nets/Coffee/README.md) | insert a coin, reject it, insert again, accept | choice, then synchronization in `Dispense` |
| [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) | enter A, then try to enter B | one process at a time |
| [Philosophers](../../examples/Petri-Nets/Philosophers/README.md) | let each philosopher eat | shared resources |
| [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) | `A takes 1`, then `B takes 2` | a deadlock |
| [TrafficLights](../../examples/Petri-Nets/TrafficLights/README.md) | go around the cycle of both lights | a conflict that protects the crossroads |
| [ElevatorControl](../../examples/Petri-Nets/ElevatorControl/README.md) | try to open the door while moving | read arcs as safety conditions |
| [ChainSaw](../../examples/Petri-Nets/ChainSaw/README.md), [LandingGear](../../examples/Petri-Nets/LandingGear/README.md), [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) | try to do the dangerous thing | safety rules as a net |
| [EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md) | answer all the calls | a real organization, its bottlenecks |
| [FourRoadJunctions](../../examples/Petri-Nets/FourRoadJunctions/README.md) | fire the sources `u0` ... `u3` | sources and sinks |

## 11. Pitfalls

- **An arc between two places** (or two transitions) is impossible: the editor inserts a node of the other kind in the middle.
- **"My transition does not fire"**: in a Petri net you must click on it, while the simulation runs (Space). Only timed nets run by themselves.
- **A forgotten token**: a sequence without its initial token never starts. Look for the double check of the P-invariants ([chapter 4](04-Analysis.md#6-p-invariants)).
- **Conflicts in a controller**: if two transitions share an input place, ask yourself who chooses. If the answer is "it depends", add the information to the net (a condition place, a priority, a color).

## 12. Going further

- C. A. Petri, *Kommunikation mit Automaten*, PhD thesis, Bonn, 1962.
- T. Murata, *Petri nets: properties, analysis and applications*, Proceedings of the IEEE, 77(4), 1989: the classic survey, with all the definitions of this chapter and the next one.
- R. David, H. Alla, *Discrete, Continuous, and Hybrid Petri Nets*, Springer: from Petri nets to GRAFCET, by French authors.
- The [bibliography](Bibliography.md) of this documentation.

---

Next: [4. Analysis of Petri nets](04-Analysis.md) — can my net deadlock? Can a place grow for ever? What never changes?
