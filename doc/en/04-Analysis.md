# 4. 🔍 Analysis of Petri nets


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/04-Analysis.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/04-Analysis.md)

**What you will learn**

- the **reachability graph** of a net, drawn by hand, then by the editor;
- the questions that matter for a real system and their precise names: **deadlock**, **dead transition**, **boundedness**, **liveness**;
- the **coverability graph** of Karp and Miller, that stays finite when a place grows for ever;
- the **incidence matrix**, the **state equation**, and what they prove without simulating;
- **P-invariants** (what never changes) and **T-invariants** (what comes back), computed by hand then by the editor;
- how to get all of it in the editor (*Analysis > Properties*) and in a terminal or a continuous integration (`tpne-check`).

**Prerequisites**: [chapter 3](03-Petri-Nets.md) (places, transitions, marking, the firing rule, the matrices `Pre` and `Post`).

---

## 1. Why analyze?

Simulating shows *one* behavior: the one you clicked. A real system meets all the others. Two robots that share two tools may work for months, then block the factory the day both take their first tool at the same instant. Analysis answers questions on **all** the behaviors:

| Question | Name of the property |
|----------|----------------------|
| Can the system block, nothing being possible any more? | **deadlock** |
| Is some part of the model never used? | **dead transition** |
| Can a buffer, a queue, a counter grow for ever? | **boundedness** |
| Can every action always happen again, whatever happened before? | **liveness** |
| Which quantities never change (one tool, three shutters at most)? | **P-invariant** |
| Which sequences of actions bring the system back to its state? | **T-invariant** |

There are two families of methods: **enumeration** (build all the reachable markings, §2 to §4) and **structural** methods (reason on the matrices, without the markings, §5 to §7). The editor uses both. Time is not considered in this chapter: the analysis of a timed net is the analysis of its untimed version, which may reach *more* markings than the timed one (the report says so).

## 2. The reachability graph

The **reachability graph** has one node per reachable marking and one edge `M → M'` labeled `t` when firing `t` in `M` gives `M'`. It is the state machine hidden in the Petri net.

### 2.1 By hand: mutual exclusion

[MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) has five places, in the order `P0` Idle A, `P1` Critical A, `P2` Mutex, `P3` Idle B, `P4` Critical B, and `M0 = (1 0 1 1 0)`.

1. In `M0`, `Enter A` (`T0`) and `Enter B` (`T2`) are enabled. Firing `T0` gives `(0 1 0 1 0)`, firing `T2` gives `(1 0 0 0 1)`.
2. In `(0 1 0 1 0)` only `Leave A` (`T1`) is enabled: `Enter B` misses the token of Mutex. `T1` gives back `M0`.
3. In `(1 0 0 0 1)` only `Leave B` (`T3`) is enabled, and gives back `M0`.

No new marking: the graph is complete, with 3 markings.

```text
                 T0 (Enter A)                    T2 (Enter B)
   0 1 0 1 0  ◄──────────────  1 0 1 1 0  ──────────────►  1 0
   0 0 1
              ──────────────►  (initial)  ◄──────────────
                 T1 (Leave A)                    T3 (Leave B)
```

| From marking | Transition | To marking |
|--------------|------------|------------|
| `(1 0 1 1 0)` initial `M0` | `T0` (Enter A) | `(0 1 0 1 0)` |
| `(0 1 0 1 0)` | `T1` (Leave A) | `(1 0 1 1 0)` |
| `(1 0 1 1 0)` | `T2` (Enter B) | `(1 0 0 0 1)` |
| `(1 0 0 0 1)` | `T3` (Leave B) | `(1 0 1 1 0)` |

Read the properties on the graph: no marking has both critical places marked, so the mutual exclusion holds; every marking has a successor, so there is no deadlock; from every marking every transition can fire again, so the net is live.

### 2.2 By the editor

In the editor, *Analysis > Properties > Graph of the markings* draws the reachability graph in a window: a box per marking, an arrow per firing labeled by its transition, the initial marking in bold, the deadlocks in red. A spring layout places the boxes (*Layout again* starts it again); drag a box or the view, the wheel zooms. A net with colors shows the colors of the tokens (`3{Phi0:1 Phi1:1 Phi2:1}`: 3 tokens, one of each color). Graphs of more than 400 markings are not drawn, only exported. Here, the 4 markings of [ColoredPhilosophers](../../examples/Extended-Petri-Nets/ColoredPhilosophers/README.md):

![Graph of the markings of ColoredPhilosophers](pics/04-Markings-ColoredPhilosophers.png)

*Export to dot...* in this window, or `tpne-check --dot graph.dot net.json` in a terminal, writes the same graph for [Graphviz](https://graphviz.org); `dot -Tpng graph.dot -o graph.png` draws it:

![Graph of the markings of MutualExclusion](pics/04-Graph-MutualExclusion.png)

*The same 3 markings, found by `tpne-check` and drawn by Graphviz. Each marking lists the tokens of `P0 ... P4`.*

### 2.3 The limit: state explosion

The reachability graph can be huge: `n` independent places with one token each give `2^n` markings, and a place that can grow for ever gives an infinite graph. The editor stops after a **maximum number of markings** (100 000 for `tpne-check`, `--max-states` to change it; 20 000 by default in the editor, the field *max markings*) and says that the result is partial. The emergency call center already has 25 443 reachable markings.

## 3. Properties

Let `R(M0)` be the set of the markings reachable from `M0`.

### 3.1 Deadlock

A **deadlock** is a reachable marking where no transition is enabled. The editor explores the graph breadth first: the first deadlock found is the **closest** to `M0`, and the editor gives the **shortest firing sequence** that leads to it.

[Deadlock](../../examples/Petri-Nets/Deadlock/README.md) is the classic: robot A takes tool 1 then tool 2, robot B takes tool 2 then tool 1.

![Graph of the markings of Deadlock](pics/04-Graph-Deadlock.png)

*The marking in red, `0 1 0 0 0 0 1 0`, is the deadlock: A holds tool 1, B holds tool 2.*

```text
$ build/linux-gcc-debug/bin/tpne-check --allow-deadlock examples/Petri-Nets/Deadlock/Deadlock.json
examples/Petri-Nets/Deadlock/Deadlock.json:
Net: Deadlock (Petri net, 8 places, 6 transitions)
Reachable markings: 6
Deadlock: yes, marking (P0=0, P1=1, P2=0, P3=0, P4=0, P5=0, P6=1, P7=0) after firing T0 (A takes 1), T3 (B takes 2)
Dead transitions: none
Bounded: yes (1-bounded)
Live: no, not live: T0 (A takes 1), T1 (A takes 2), T2 (A releases), T3 (B takes 2), T4 (B takes 1), T5 (B releases)
P-invariants:
  P0 (A idle) + P1 (A has 1) + P2 (A works) = 1
  P1 (A has 1) + P2 (A works) + P3 (Tool 1) + P7 (B works) = 1
  P5 (B idle) + P6 (B has 2) + P7 (B works) = 1
  P2 (A works) + P4 (Tool 2) + P6 (B has 2) + P7 (B works) = 1
T-invariants:
  T0 (A takes 1) + T1 (A takes 2) + T2 (A releases)
  T3 (B takes 2) + T4 (B takes 1) + T5 (B releases)
```

The cure is to change the net: both robots take the tools in the same order, or take both at once (one transition with the two tools as input places, as the [philosophers](../../examples/Petri-Nets/Philosophers/README.md) do).

Sometimes a deadlock is wanted: the end of a job. [FirstNet](../../examples/Petri-Nets/FirstNet/README.md) ends in `Done`, the [pick and place](../../examples/Runtime/PickAndPlace/README.md) of the runtime in `Done`. `--allow-deadlock` tells `tpne-check` not to fail on them.

### 3.2 Dead transitions

A transition is **dead** when it is enabled in no reachable marking: it can never fire. It is often a modeling error (a missing token, an arc in the wrong direction). The opposite, "fires at least once in some behavior", is called **quasi-liveness** (liveness of level L1).

### 3.3 Boundedness

A place is **k-bounded** when it never holds more than `k` tokens in any reachable marking; a net is bounded when all its places are. A **safe** net is 1-bounded: each place is a boolean condition (true when marked). The editor gives the bound of each place (tab *Bounds*) and the bound of the net: `1-bounded` for the deadlock example, `3-bounded` for [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) (the place `Remaining` holds 3 tokens). A bounded net has a finite reachability graph. An unbounded place is a buffer without limit: memory that will overflow, a queue that will never empty.

### 3.4 Liveness

A transition is **live** (level L4) when, from every reachable marking, some firing sequence makes it enabled again: whatever happened, it can still happen. A net is live when all its transitions are. Liveness is stronger than the absence of deadlock: a net can run for ever in a loop that leaves some transitions out. The editor computes it on the complete reachability graph: a transition is live when it labels an edge inside every terminal strongly connected component of the graph (a group of markings that the system cannot leave). When the graph is not complete, liveness is "unknown".

| Example | Deadlock | Bounded | Live |
|---------|:--------:|:-------:|:----:|
| [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) | none | 1 | yes |
| [Philosophers](../../examples/Petri-Nets/Philosophers/README.md) | none | 1 | yes |
| [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) | yes | 1 | no |
| [Conflict](../../examples/Petri-Nets/Conflict/README.md) | yes (after the choice) | 1 | no |
| [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) | none | 3 | yes |
| [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) | none found | no (`Buffer`) | unknown |

The editor does not compute **reversibility** (can the initial marking always be reached again?) nor home states; for a live net whose reachability graph is one strongly connected component, as MutualExclusion, it holds.

## 4. The coverability graph (Karp-Miller)

When a place grows for ever, the reachability graph is infinite. In 1969, Karp and Miller built a finite graph that still answers the boundedness question: the **coverability graph**. The idea: if the exploration reaches a marking `M'` that is **larger** than a marking `M` on its path (`M' ≥ M` place by place, and `M' ≠ M`), the firing sequence from `M` to `M'` can be repeated as many times as wanted, and the places that grew can hold as many tokens as wanted: they are replaced by `ω` (omega, "as many as you want").

[Unbounded](../../examples/Timed-Petri-Nets/Unbounded/README.md) has one transition `T0` that gives back the token of `P0` and adds one to `P1`:

```text
M0 = (1 0)  ──T0──►  (1 1)  ≥ (1 0) and larger in P1:  P1 becomes ω
(1 ω)  ──T0──►  (1 ω)                                  (ω + 1 = ω)
```

The coverability graph computed by the library has these two markings:

```text
digraph "Unbounded" {
  M0 [label="1 0", penwidth=2];
  M1 [label="1 w"];
  M0 -> M1 [label="T0"];
  M1 -> M1 [label="T0"];
}
```

A place is unbounded exactly when an `ω` appears in it. For the producer and the consumer, the coverability graph has 46 markings and `ω` in `Buffer` only. The editor builds it by itself when the reachability graph is too big, to tell whether the net is bounded:

```text
$ build/linux-gcc-debug/bin/tpne-check examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json:
Net: Producer Consumer (Petri net, 5 places, 4 transitions)
Reachable markings: 100000 (stopped)
Deadlock: none found
Dead transitions: none
Bounded: no, unbounded: P2 (Buffer)
Live: unknown
P-invariants:
  P0 (Producer ready) + P1 (Idle) = 2
  P3 (Accepted) + P4 (Ready) = 2
T-invariants:
  T0 (Send) + T1 (Produce) + T2 (Accept) + T3 (Consume)
Note: Stopped after 100000 markings: no deadlock found is not a proof, the liveness is unknown.
```

Two limits:

- The coverability graph loses information: `ω` means "unbounded", not "any number". It proves boundedness and dead transitions, not deadlocks nor liveness.
- Karp-Miller needs a **monotonic** firing rule: more tokens never disable a transition. Inhibitor arcs, reset arcs and capacities break it, and the editor refuses them ("Karp-Miller does not apply to nets with inhibitor arcs"); see [chapter 5](05-Extended-Petri-Nets.md#8-what-the-extensions-cost).

## 5. The incidence matrix and the state equation

Enumeration grows with the number of markings. Structural methods only look at the matrices of the net, whatever its marking. The **incidence matrix** is

```text
C = Post − Pre        C(p, t) = tokens given to p by t − tokens taken from p by t
```

For MutualExclusion (places `P0` Idle A, `P1` Critical A, `P2` Mutex, `P3` Idle B, `P4` Critical B; transitions `T0` Enter A, `T1` Leave A, `T2` Enter B, `T3` Leave B):

```text
            T0  T1  T2  T3
       P0 | -1  +1   0   0 |
       P1 | +1  -1   0   0 |
  C =  P2 | -1  +1  -1  +1 |
       P3 |  0   0  -1  +1 |
       P4 |  0   0  +1  -1 |
```

Column `T0` reads: Enter A takes a token from Idle A and from Mutex, gives one to Critical A. If a firing sequence `σ` fires each transition `t` `σ(t)` times (the **firing count vector**, or Parikh vector), every firing adds its column, whatever the order, so

```text
M = M0 + C · σ             (the state equation)
```

Check with `σ = T0` (fire Enter A once): `M = (1,0,1,1,0) + (−1,+1,−1,0,0) = (0,1,0,1,0)`, the marking of §2.1.

The state equation gives a **necessary** condition: if `M` is reachable, the equation has a solution `σ ≥ 0` in integers. Contrapositive: **no solution proves that `M` is not reachable**. Can both processes be critical, `M = (0,1,0,0,1)`? The rows of `P0`, `P3` and `P2` ask

```text
P0:  1 − σ(T0) + σ(T1) = 0             so  σ(T0) = σ(T1) + 1
P3:  1 − σ(T2) + σ(T3) = 0             so  σ(T2) = σ(T3) + 1
P2:  1 − σ(T0) + σ(T1) − σ(T2) + σ(T3) = 1 − 1 − 1 = −1 ≠ 0
```

No solution: the mutual exclusion is proved for every behavior, without building a single marking. The condition is not sufficient: a solution does not prove that `M` is reachable, because the equation ignores the order of the firings. Think of a transition `t` with a self-loop on an empty place `p` and an output place `q`: its column is 0 for `p` and +1 for `q`, so `(0, 1) = (0, 0) + C · t` has a solution, but `t` is never enabled.

## 6. P-invariants

A **P-invariant** (place invariant) is a vector `y` of weights on the places such that

```text
y · C = 0
```

Multiply the state equation by `y`: `y · M = y · M0 + y · C · σ = y · M0`. The weighted sum of the tokens of these places **never changes**, whatever fires. The editor gives the **minimal semi-positive** P-invariants (also called P-semiflows): `y ≥ 0`, with a minimal set of places (its **support**), computed by the algorithm of Farkas (1902), adapted to Petri nets by Martinez and Silva.

### 6.1 By hand

In the matrix `C` of MutualExclusion, look for weights that cancel every column.

- `y = (1, 1, 0, 0, 0)` (Idle A + Critical A): column `T0` gives `−1 + 1 = 0`, `T1` gives `+1 − 1 = 0`, `T2` and `T3` give 0. It is a P-invariant: `Idle A + Critical A = 1` (process A is in exactly one state).
- `y = (0, 0, 0, 1, 1)`: `Idle B + Critical B = 1`.
- `y = (0, 1, 1, 0, 1)`: column `T0` gives `+1 − 1 = 0`, `T1` gives `−1 + 1 = 0`, `T2` gives `−1 + 1 = 0`, `T3` gives `+1 − 1 = 0`. So `Critical A + Mutex + Critical B = 1`.

The last one *is* the mutual exclusion: since tokens are never negative, `Critical A + Critical B ≤ 1`. The editor finds the same three:

```text
$ build/linux-gcc-debug/bin/tpne-check examples/Petri-Nets/MutualExclusion/MutualExclusion.json
examples/Petri-Nets/MutualExclusion/MutualExclusion.json:
Net: Mutual exclusion (Petri net, 5 places, 4 transitions)
Reachable markings: 3
Deadlock: none
Dead transitions: none
Bounded: yes (1-bounded)
Live: yes
P-invariants:
  P0 (Idle A) + P1 (Critical A) = 1
  P3 (Idle B) + P4 (Critical B) = 1
  P1 (Critical A) + P2 (Mutex) + P4 (Critical B) = 1
T-invariants:
  T0 (Enter A) + T1 (Leave A)
  T2 (Enter B) + T3 (Leave B)
```

### 6.2 What P-invariants tell

- **Conservation**: each resource of the [philosophers](../../examples/Petri-Nets/Philosophers/README.md) is an invariant (`P1 (Phi1 Eating) + P3 (Phi0 Eating) + P6 (Fork) = 1`: the fork is held by Phi0, by Phi1, or free).
- **Safety rules**: `P1 + P3 + P5 + P7 (the shutters down) + P8 (Remaining) = 3` in [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md): at most three shutters are down, a way out always stays open.
- **Weights**: invariants can have weights. In [Weights](../../examples/Extended-Petri-Nets/Weights/README.md), `2 P1 (O2) + P2 (H2O) = 4`: each molecule of water holds half a molecule of oxygen, the atoms are conserved.
- **Boundedness**: a place covered by a P-invariant with a positive weight is bounded, whatever the marking. `Buffer` of ProducerConsumer is in no P-invariant: it is the unbounded place.
- **Modeling errors**: a place that should be conserved and is not in any invariant is a missing arc. A sum that is not the expected number (2 instead of 1) is a token too many in `M0`. In ProducerConsumer, `Producer ready + Idle = 2`: there are two producers.

## 7. T-invariants

A **T-invariant** is a vector `x ≥ 0` of firing counts such that

```text
C · x = 0
```

By the state equation, a firing sequence whose firing count vector is `x` gives back the marking where it started: it is a **cycle** of the behavior. In MutualExclusion, `x = (1, 1, 0, 0)` (Enter A, Leave A): column `T0` plus column `T1` is zero. T-invariants are the loops of a system; a transition in no T-invariant cannot fire infinitely often in a bounded net (it can only fire a finite number of times). [Conflict](../../examples/Petri-Nets/Conflict/README.md) has none: nothing comes back after the choice.

A T-invariant is a candidate cycle: the order of the firings is not given, and some orders may not be possible from the current marking (in [Deadlock](../../examples/Petri-Nets/Deadlock/README.md), `T0 + T1 + T2` is a cycle of robot A, but not after `B takes 2`).

**Only the normal arcs** count in `C`: with inhibitor or reset arcs the editor writes "Only the normal arcs are considered: the invariants may not hold with inhibitor or reset arcs" ([chapter 5](05-Extended-Petri-Nets.md#8-what-the-extensions-cost)).

## 8. In the editor

### 8.1 Analysis > Properties

*Analysis > Properties* brings the tab **Summary** of the *Analysis* window in front (the window is always there), for every type of net. The properties have four tabs: *Summary*, *Bounds*, *Invariants* and *Report*; each one starts with *Compute*, *max markings*, *Graph of the markings* and *Show on the net*. For a small net (60 nodes or fewer) it is computed at once, and again after each change: draw and watch the properties follow. For a bigger net, press **Compute**; *max markings* sets the limit of the exploration.

![Properties of the deadlock example](pics/04-Properties-Deadlock.png)

*The properties of the two robots. The summary: 6 markings (all), a deadlock, no dead transition, 1-bounded, not live. With *Show on the net*, the transitions that are not live are framed on the net.*

| Tab or button | What it shows |
|---------------|---------------|
| Summary | markings explored (and whether all were), deadlock, dead transitions, bounded, live, in green or red; then the deadlock marking, the shortest firing sequence to it, and *Replay the path in the Journal* |
| Bounds | the maximum number of tokens of each place, or unbounded |
| Invariants | the P- and T-invariants; a click on one marks its nodes on the net |
| Report | the same text as `tpne-check` (*Copy the report*) |
| *Show on the net* | marks the dead transitions, the transitions not live and the unbounded places |
| *Graph of the markings* | draws the reachability graph in a window (*Export to dot...* saves it for Graphviz) |

*Replay the path in the Journal* fills the *Journal* with the firings that lead to the deadlock: its replay slider shows the marking after each one, step by step ([chapter 13](13-Debugging.md)).

![Properties of the producer and consumer](pics/04-Properties-ProducerConsumer.png)

*The producer and the consumer: the exploration stopped at 20 000 markings, Karp-Miller says that `Buffer` (P2) is unbounded, marked in red on the net.*

![Properties of mutual exclusion](pics/04-Properties-MutualExclusion.png)

*Mutual exclusion: live, 1-bounded, and the three P-invariants of §5.1.*

### 8.2 tpne-check in a terminal or a CI

```text
tpne-check [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json ...
```

| Option | Effect |
|--------|--------|
| `--max-states N` | stop the exploration after `N` markings (default 100000) |
| `--allow-deadlock` | a deadlock is not an error (the goal of a job) |
| `--dot graph.dot` | write the reachability graph of the last net for Graphviz |

It prints the report of each net and returns **0** when no net can deadlock, **1** when one can (unless `--allow-deadlock`), **2** for a bad usage or a file that cannot be read. It reads the formats of the editor (`.json`, `.pnml`, `.grml`, `.teg`). The continuous integration of this project runs it on examples, so a change that introduces a deadlock fails the build:

```sh
build/linux-gcc-debug/bin/tpne-check examples/Petri-Nets/Philosophers/Philosophers.json examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
build/linux-gcc-debug/bin/tpne-check --allow-deadlock examples/Runtime/PickAndPlace/PickAndPlace.json
```

Do the same in your project: draw the net of your controller, commit it, check it at each commit. The same functions are in the C++ library (`properties()`, `pInvariants()`, `tInvariants()`, `reachabilityGraph()`, `coverabilityGraph()`, `report()` of `Analysis/Analysis.hpp`, [chapter 16](16-Library.md)).

### 8.3 What the analysis does not consider

The report adds a note when it ignores something:

- **durations** of a timed Petri net: "the timed net may reach fewer markings"; a deadlock found may be impossible with the real durations;
- **GRAFCET rules** (receptivities, simultaneous firing): the analysis of a GRAFCET is the analysis of its Petri net;
- **colors** of the tokens: they are considered, as the runtime fires them ("The colors of the tokens are considered"). A marking is the tokens of each color in each place, and a firing gives one branch for each color its result can have: the colors asked by the arcs after the transition (`Success` if none is). A function of the runtime that returns another color is not considered. If the net has too many markings, the coverability graph of the bounds does not consider the colors (a note says so).

This matters: the analysis without colors finds markings that the runtime never reaches. In [ColoredPhilosophers](../../examples/Extended-Petri-Nets/ColoredPhilosophers/README.md), it finds 10 markings, where `Phi0 eats` takes the token of `Phi1`; with the colors there are 4 (at rest, or one philosopher eating).

## 9. Try it

| Example | Do | Observe |
|---------|----|---------|
| [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) | open the Properties, then add a token to Mutex | the invariant becomes `= 2`: the exclusion is lost |
| [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) | *Replay the path in the Journal*; then make robot B take tool 1 first | the deadlock disappears |
| [Philosophers](../../examples/Petri-Nets/Philosophers/README.md) | click on each invariant | the nodes of each resource |
| [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) | change `Remaining` to 4 | the safety rule is gone, the invariant says `= 4` |
| [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) | right click on `Buffer` > *Max tokens* | the graph becomes finite and the net bounded (Karp-Miller is no longer needed) |
| [EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md) | *Compute* | 25 443 markings; the end, all calls handled, is a deadlock |

## 10. Pitfalls

- **"No deadlock found" is not a proof** when the exploration stopped: read the summary (*all* or *stopped*).
- **Unbounded because of a source**: a transition without input place can fire for ever, every place after it is unbounded unless something limits it (a capacity, an arc back).
- **Analyzing a timed net**: the untimed net may deadlock where the timed one does not; check the deadlock path against the durations.
- **Invariants and extensions**: with inhibitor or reset arcs, an invariant of the normal arcs may be false.

## 11. Going further

- T. Murata, *Petri nets: properties, analysis and applications*, 1989.
- R. M. Karp, R. E. Miller, *Parallel program schemata*, 1969 (the coverability tree).
- J. Martinez, M. Silva, *A simple and fast algorithm to obtain all invariants of a generalized Petri net*, 1982 (Farkas for Petri nets).
- The [bibliography](Bibliography.md).

---

Next: [5. Extended Petri nets](05-Extended-Petri-Nets.md) — weights, capacities, inhibitor and reset arcs, priorities, colors.
