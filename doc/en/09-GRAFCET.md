# 9. 🔘 GRAFCET


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/09-GRAFCET.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/09-GRAFCET.md)

**What you will learn**

- where the GRAFCET comes from, and how it differs from a Petri net and from the SFC language of the PLCs;
- its elements: steps, transitions, receptivities, actions, links;
- the five rules of evolution of the standard IEC 60848, and the search of the stable situation;
- the structures: sequence, choice, parallelism, loops;
- how to write receptivities (logic, edges, time, comparisons) and actions (the ten qualifiers, conditional actions, forcing orders);
- how the editor runs a GRAFCET: the control panel, the outputs, the timing diagram, the timers;
- to draw your own GRAFCET from scratch.

**Prerequisites**: [3. Petri nets](03-Petri-Nets.md) (places, transitions, tokens, the firing rule). The time of [chapter 6](06-Timed-Petri-Nets.md) is not needed: the GRAFCET has its own time operators.

---

## 1. Where the GRAFCET comes from

In the 1970s, the automation of machines was described with relay diagrams and long texts that nobody could check. In 1977, a working group of the AFCET (the French association of computer science and control) and of the ADEPA (an agency for the automation of the French industry) proposed the **GRAFCET** (*GRAphe Fonctionnel de Commande Étapes-Transitions*, the functional graph of control by steps and transitions). The idea was to keep the graphical clarity of the Petri nets, but to make each place a state of the machine and to read the sensors on the transitions.

The GRAFCET became the French standard NF C03-190, then the international standard **IEC 60848** (1988, last edition 2013). It is taught in every French technical school: when a French engineer draws "a sequence", it is a GRAFCET.

Do not confuse it with the **SFC** (*Sequential Function Chart*) of the standard IEC 61131-3, which is one of the five programming languages of the PLCs (Programmable Logic Controllers). The SFC was built from the GRAFCET, but:

| | GRAFCET (IEC 60848) | SFC (IEC 61131-3) |
|-|---------------------|-------------------|
| What it is | a language of **specification**: what the machine shall do | a **programming** language of a PLC |
| Evolution | all the transitions that can fire fire together; the graphs evolve until they are stable before the actions are computed | one evolution per scan of the PLC; the details depend on the vendor |
| Actions | qualifiers, conditional actions, forcing orders between graphs | qualifiers, action blocks written in other languages (ST, ladder ...) |

This editor follows **IEC 60848**: you specify the behavior, run it on a virtual machine (the control panel), then generate its code ([chapter 12](12-Code-Generation.md)).

## 2. A GRAFCET is an interpreted Petri net

A GRAFCET is a Petri net with three changes:

1. a place is a **step**, and it holds one token at most: the step is **active** or **inactive** (the net is *safe*, in the words of [chapter 4](04-Analysis.md));
2. a transition has a **receptivity**: a boolean condition on the inputs of the machine (buttons, sensors), on the time and on the other steps. The net is *interpreted*: the outside world decides when a transition fires;
3. a step does **actions** while it is active: it drives the outputs of the machine (motors, valves, lamps).

```text
   ╔════╗
   ║ 0  ║ Wait                 <- initial step (double square): active at the start
   ╚═╤══╝
     ┼  Dcy                    <- transition and its receptivity: the button Dcy
   ┌─┴──┐ ┌───┬───────┐
   │ 1  ├─┤ N │ Motor │        <- step 1 and its action: Motor runs while step 1 is active
   └─┬──┘ └───┴───────┘
     ┼  30s/X1                 <- 30 s after step 1 became active
     ⋮
```

| Element | Drawn | Meaning |
|---------|-------|---------|
| **Step** | a square with its number | a state of the machine. The step `n` is written `Xn` in the receptivities: `X3` is true while step 3 is active |
| **Initial step** | a double square | active at the start (it has a token in the file) |
| **Transition** | a bar across the link | the passage from the steps above it to the steps below it |
| **Receptivity** | the text right of the bar | the condition of the transition |
| **Action** | a box right of its step | what the machine does while the step is active (or when it changes, with a qualifier) |
| **Link** | a vertical line | it goes downwards; a link going up has an arrow |
| **Input** | a name in a receptivity | what the GRAFCET reads: `Dcy`, `s1`, `Temperature` |
| **Output** | the name of an action | what the GRAFCET drives: `Motor` |

In the editor, the big number in a step is its identifier (the `n` of `Xn`); the small text under it is its caption. A transition shows its number in parentheses on its left, `(0)`, when *View > Transition identifiers* is checked.

## 3. The five rules of evolution

IEC 60848 gives five rules. Here they are, each with a tiny example, and how the editor applies it (`GrafcetEngine`, in [`src/PetriNet/GrafcetEngine.cpp`](../src/PetriNet/GrafcetEngine.cpp)).

**Rule 1 — Initial situation.** At the start, the initial steps are active, and only them. In the file, an initial step has one token; in the editor, **+** on a step makes it initial (double square).

**Rule 2 — A transition can fire** when all the steps just above it are active (the transition is **enabled**) *and* its receptivity is true. A transition that can fire **shall** fire: there is no choice, unlike a Petri net.

```text
   [1] active, receptivity a = 1      ->  the transition fires
   [1] active, receptivity a = 0      ->  it waits
   [1] inactive, receptivity a = 1    ->  nothing: the transition is not enabled
```

In the editor, a receptivity that is true is drawn in orange, even when its transition is not enabled: this shows at once what is waiting for what.

**Rule 3 — Firing a transition** deactivates all the steps just above it and activates all the steps just below it, at the same time.

**Rule 4 — Transitions that can fire together, fire together.** If two transitions can fire at the same instant, both fire. There is no order and no conflict to solve.

**Rule 5 — A step deactivated and activated at the same time stays active.** It happens when a transition goes back to the step it leaves, or when two transitions fire together around the same step. The editor first deactivates all the steps above the firing transitions, then activates all the steps below them: the step ends active.

### 3.1 The stable situation

A cycle of the engine (`step(dt)`) does this:

1. let `dt` seconds pass for the timers (`3s/X2`, the delays);
2. apply the **forcing orders** of the active steps (§7);
3. **evolve**: find all the transitions that can fire, fire them together (rules 2 to 5), and again, until no transition can fire. The graphs are then **stable**;
4. compute the **actions** from the stable situation.

The steps passed through during step 3 are **transient**: they were active during zero time, so their continuous actions are not run. This is what the standard asks: a motor shall not start for a microsecond because the GRAFCET went through a step. Read the dryer of [chapter 10](10-GEMMA.md): with the button `Start` held, the graph `Control` goes from step 3 to step 4 and, *in the same cycle*, the graph `Production` sees `Control:X4` and goes to step 1. Both happen in one cycle because the search of the stable situation evolves all the graphs together, again and again.

An **edge** (`↑a`, §5) is true during one evolution only (one *instant*): pressing a button seen with `↑Ack` fires one transition once, even if the graph comes back to a step waiting for `↑Ack` during the same cycle.

If the graphs never become stable (after 256 evolutions), the engine stops the search and the console warns: *"The GRAFCET does not reach a stable situation after 256 evolutions: is there a loop of transitions always true?"*. See the pitfalls (§12).

## 4. The structures

A GRAFCET is built from a few structures. Each one has an example in [`examples/GRAFCET`](../../examples/GRAFCET).

### 4.1 Sequence and loop

Steps one after the other, the last transition going back to the first step (a link going up, with an arrow): the [German traffic light](../../examples/GRAFCET/GermanTrafficLights/README.md) is a sequence of four steps, green, yellow, red, red and yellow.

### 4.2 Choice: OR divergence and OR convergence

A step followed by several transitions is an **OR divergence** (drawn with a single horizontal line): the GRAFCET takes the branch whose receptivity is true. The branches meet again on a step preceded by several transitions: an **OR convergence**.

![Exclusive choice](pics/09-ExclusiveChoice-running.png)

*[ExclusiveChoice](../../examples/GRAFCET/ExclusiveChoice/README.md): `A . B` goes to step 1, `A . /B` to step 2. The `/A` receptivities are true (orange) but their transitions are not enabled: steps 1 and 2 are inactive.*

By rule 4, if two receptivities of an OR divergence are true together, **both** transitions fire, and both branches become active: this is almost never what you want. Make the receptivities **exclusive**: `A . B` and `A . /B` can never be true together. A common writing is a priority: `b1` on one branch, `b2 . /b1` on the other.

### 4.3 Parallelism: AND divergence and AND convergence

A transition followed by several steps is an **AND divergence** (drawn with a double horizontal line): all the branches start together and run in parallel. A transition preceded by several steps is an **AND convergence** (a double line too): it waits until all the branches have reached their last step, then fires once.

![Parallel sequences](pics/09-ParallelSequences.png)

*[ParallelSequences](../../examples/GRAFCET/ParallelSequences/README.md): after `m`, steps 1 and 4 are active together. The last transition, `true`, waits for steps 3 and 8: the receptivity is always true, the AND convergence alone makes it wait.*

### 4.4 Jumps

A jump is a link that skips steps (forwards) or goes back (backwards, a loop inside a branch). In ParallelSequences, `c0 . m` jumps from step 6 to step 7, and `c1` comes back to step 6: a loop inside the right branch, left by `c0 . /m`.

## 5. Receptivities

### 5.1 Syntax

| Write | Means | Also accepted |
|-------|-------|---------------|
| `a . b` | a and b | `a * b`, `a and b` |
| `a + b` | a or b | `a \| b`, `a or b` |
| `/a` | not a | `!a`, `not a` |
| `( ... )` | parentheses | |
| `1`, `0` | always true, never true | `true`, `false` |
| (empty) | always true | |
| `X3` | step 3 of this graph is active | |
| `Safety:X3` | step 3 of the graph named Safety is active | |
| `↑a` | rising edge: a has just become true (true during one instant) | `^a`, `rising(a)` |
| `↓a` | falling edge: a has just become false | `falling(a)` |
| `3s/X2` | true 3 s after step 2 became active | units `ms`, `s`, `min`, `h` |
| `3s/a/7s` | true 3 s after a became true, false 7 s after a became false | |
| `[count >= 3]` | comparison of an integer input | `=`, `!=`, `<`, `<=`, `>`, `>=` |
| `Dcy`, `sensor_2` | an input of the machine | names with arguments, like `holding(cube)` |

**Precedence**: `not` first, then `and`, then `or`. So `a + b . /c` means `a + (b . (/c))`. Use parentheses when in doubt.

Old files wrote receptivities in reverse polish notation (`a X0 ! .` for `a . /X0`); the editor converts them when it opens the file.

### 5.2 Time

The **time operator** `t1/e/t2` (IEC 60848) watches an expression `e`:

```text
      e   ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|________________
t1/e/t2   ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|______
              <- t1 ->                <- t2 ->
```

The most common use is `t1/Xn`: true `t1` after step `n` became active. It is how a GRAFCET waits: `30s/X1` lets a motor run 30 seconds.

A transition can also have a **delay** (the field *Delay* of the *Transitions* panel): it fires only after being able to fire during this time. `Dcy` with a delay of 2 s is the same as `2s/Dcy`.

While the simulation runs, each timer that counts is drawn under its receptivity as a bar, `1.0 / 3 s`, and listed in the *Transitions* panel; the *Steps* panel writes since when each active step is active.

### 5.3 In the editor

Double click a transition to write its receptivity: the editor checks the syntax while you type, and an error stays red in the *Transitions* panel. *Syntax help* (in that panel) and *Help > GRAFCET receptivities* show the table above. Every name that is not a step and not a keyword becomes an input of the control panel (§9).

## 6. Actions

An action has a **qualifier**, a **name** (its output), an optional **condition** and, for the timed qualifiers, a **duration** `d`. Several actions of the same name drive the same output: the output is on while one of them is on.

### 6.1 The ten qualifiers

```text
 step active     ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________
 N               ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________   while the step is active
 D  (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________   d after the activation
 L  (d)          ____|‾‾‾‾‾‾‾|____________________________________   during d at most
 P1              ____|‾|_______________________________________   one cycle at the activation
 P0              __________________________________|‾|_________   one cycle at the deactivation
 S               ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   until an R of the same name
 SD (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   d after the activation, then stored
 DS (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   stored if the step is still active after d
 SL (d)          ____|‾‾‾‾‾‾‾|____________________________________   during d, even if the step is left
```

| Qualifier | Name | The output is on ... |
|-----------|------|----------------------|
| `N` | continuous | while the step is active |
| `S` | set (stored) | from the activation of the step, until an action `R` of the same name |
| `R` | reset | (it switches off the stored action of the same name, while its step is active) |
| `D` | delayed | `d` seconds after the activation, while the step stays active |
| `L` | limited | during the first `d` seconds of the step |
| `SD` | stored then delayed | `d` seconds after the activation, even if the step was left; then stored until `R` |
| `DS` | delayed then stored | if the step is still active after `d` seconds; then stored until `R` |
| `SL` | stored and limited | during `d` seconds from the activation, even if the step was left |
| `P1` | pulse on activation | during one cycle when the step becomes active |
| `P0` | pulse on deactivation | during one cycle when the step becomes inactive |

*Help > GRAFCET actions (qualifiers)* draws the timing diagram of each qualifier; in the *Actions* panel, choosing a qualifier shows its own diagram (*Qualifiers help*).

### 6.2 Conditional actions

An action can have a **condition** written like a receptivity: the action runs only while its step is active *and* its condition is true. It is drawn `if Right: Right Motor`. In the dryer of [chapter 10](10-GEMMA.md), the step "Manual run" has `N Right Motor` with the condition `Right`: the motor turns only while the operator holds the button.

### 6.3 Example: a pneumatic cylinder

![GRAFCET actions](pics/09-GrafcetActions.png)

*[GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md): `N V1+` while extending, `D Timer1` 2 s after step 2, `S Lamp1` at step 2 and `R Lamp1` at step 3, `N Buzzer` and `L AlarmLamp` (5 s) at the alarm. The *Actions* panel lists the actions of each step.*

## 7. Forcing orders

A **forcing order** is an action of one graph that imposes a situation on another graph. While the step holding it is active, the forced graph takes the given situation and **does not evolve**: forcing has priority over the rules of evolution. This is how a safety graph stops everything.

| Order | The graph Production ... |
|-------|--------------------------|
| `Production{init}` | takes its initial situation |
| `Production{2,5}` | has steps 2 and 5 active, and only them |
| `Production{}` | has no active step |
| `Production{*}` | is frozen in its current situation |

The colon of the old writing is accepted: `Production:{init}`. A forcing order is drawn in a double frame; an action holding only forcing orders has no name. In the *Actions* panel, *Add order* adds one. The forcing orders are applied at the beginning of each cycle, from the situation of the previous cycle (§3.1). [Chapter 10](10-GEMMA.md) uses them to build a GEMMA.

## 8. Several graphs in one document

A document can hold several GRAFCETs (*Edit > Add a net to the document > GRAFCET*, or *File > New > GEMMA (3 GRAFCETs)*). They run together, with one engine: one search of the stable situation for all of them. A graph reads the steps of another with `Name:Xn` and forces it with `Name{...}`. The name of a graph (and its description) is changed with *Edit* in the *Document* tab, at the bottom next to the *Console*.

## 9. Inputs, outputs and the control panel

The **inputs** are the names read by the receptivities and the conditions. The **outputs** are the names of the actions. When the simulation starts, the editor collects them and shows:

- the **Control panel**: the desk of the machine, one widget per input;
- the **Outputs** window: one actuator per output;
- the **Timing diagram** (a tab next to the console): the steps, inputs and outputs over time.

![German traffic lights](pics/09-GermanTrafficLights-running.png)

*The [German traffic light](../../examples/GRAFCET/GermanTrafficLights/README.md) after 5 s: step 2 is active, its timer `3s/X2` counts (`1.0 / 3 s`), the red light is on in the Outputs window.*

By default an input is a switch. *Edit* on the control panel chooses how each input is drawn and what it does; this is saved in the file ([chapter 15](15-File-Formats.md)):

| Kind | Drawn as | Behavior |
|------|----------|----------|
| `switch` | rotary switch 0 / 1 | a click turns it on or off, and it stays |
| `button` | push button | on while pressed (one cycle at least); a right click keeps it pressed |
| `pulse` | square button | on during one cycle per click |
| `emergency` | red mushroom on yellow | pushed (on) until a second click pulls it back |
| `selector` | rotary selector | its positions are the inputs of the same group; one only is on |
| `value` | gauge and slider | an integer (temperature, level, counter), read with `[Temperature >= 80]`; it can follow an output: it rises while the output is on, falls otherwise |
| `clock` | blinking light | on during half of its period |

| Output kind | Drawn as |
|-------------|----------|
| `light` | a pilot light of the color of its action |
| `motor` | a fan that turns while on |
| `heater` | a resistor that glows while on |
| `valve` | a cylinder whose rod goes out while on |
| `horn` | sound waves while on |

While the simulation is stopped, a click on the panel changes the **initial value** of the input (saved with the document). While it runs, a click acts at once. To change several inputs **in the same instant** (a selector and a button), check *Apply together*: the changes wait until *Apply* (or **F7**). *Fit* draws smaller buttons to see all of them.

The **Timing diagram** has check boxes to choose the rows (*Steps*, *Inputs*, *Outputs*), *Only what changed* to hide the rows that never moved, and *Follow* to show the last seconds; the mouse on the diagram gives the time and the value. Steps are blue, inputs orange, outputs green, a `value` input is a curve.

![The timing diagram of the German traffic lights](pics/09-GermanTrafficLights-timing.png)

*The German traffic lights after 9 s: at the bottom, the timing diagram of the four steps and of the outputs; under `3s/X0`, the timer of the transition counts `1.0 / 3 s`; on the right of the console, the Outputs.*

The speed of the toolbar (x0.1 to x50) also scales the GRAFCET time: at x10, `30s/X1` lasts 3 real seconds. **P** pauses the simulation.

## 10. Tutorial: your first GRAFCET

We draw a drill: the operator presses `Dcy`, the drill goes down until the sensor `low`, waits 2 s at the bottom, goes up until `high`, and lights a lamp during 1 s at the end.

1. *File > New > GRAFCET*.
2. **Left click** in the void: a step `0`. Left click again under it, four times: steps `1` to `4`. In a GRAFCET, a click adds a step.
3. **Left drag** from step 0 to step 1: the editor inserts a transition between two steps of the same kind (a link step → step is not possible). Do it again from 1 to 2, 2 to 3, 3 to 4, and from 4 back to 0 (the link going up gets an arrow).
4. Put the mouse on step 0 and press **+**: it becomes the initial step (double square).
5. **Double click** each transition and write its receptivity: `↑Dcy` (or `^Dcy`), `low`, `2s/X2`, `high`, `1s/X4`.
6. **Right click** step 1 > *Add an action*; in the *Actions* panel set the qualifier `N` and the name `Down`. Step 3: `N Up`. Step 4: `N Lamp` (or `L Lamp` with a duration of 1 s).
7. *Edit > Arrange top-down* puts the steps in a column, the way a GRAFCET is drawn.
8. Press **Space**. The control panel shows `Dcy`, `low`, `high` as switches: on the panel, *Edit* and make `Dcy` a `button`.
9. Press `Dcy`: step 1, the output `Down` is on. Turn `low` on: step 2, the timer `x / 2 s` counts under `2s/X2`, then step 3. Turn `low` off and `high` on: step 4, the lamp, then back to step 0.
10. Open the *Timing diagram* tab and read the whole cycle. Save with **Ctrl+S**.

To run it on a real machine, generate its C++ code ([chapter 12](12-Code-Generation.md)) or call the engine from your program ([chapter 16](16-Library.md)).

## 11. Try it

| Example | What to do | What to observe |
|---------|------------|-----------------|
| [GermanTrafficLights](../../examples/GRAFCET/GermanTrafficLights/README.md) | press Space | a sequence driven by time only; step 3 has two actions; the timing diagram |
| [ExclusiveChoice](../../examples/GRAFCET/ExclusiveChoice/README.md) | Space, then A with and without B | an OR divergence with exclusive receptivities |
| [ParallelSequences](../../examples/GRAFCET/ParallelSequences/README.md) | Space, `m`, then the sensors | an AND divergence, a loop, an AND convergence |
| [GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md) | Space, `Start`, `s1`, `s0`; `Error`, `Reset` | the qualifiers N, D, S, R, L in the Outputs and the timing diagram |
| [GEMMA-Dryer](../../examples/GRAFCET/GEMMA-Dryer/README.md) | see [chapter 10](10-GEMMA.md) | three graphs, forcing orders, a control panel with all the kinds of inputs |

## 12. Pitfalls

- **An OR divergence that is not exclusive.** Two receptivities true together start both branches (rule 4): two motors in opposite directions. Write `b1` and `b2 . /b1`.
- **An unstable GRAFCET.** A loop of transitions whose receptivities are true together makes the graph turn inside one cycle, for ever. Open [GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md), press Space and turn `Start`, `Error` and `Reset` on together: the graph goes 0 → 1 (`Start`) → 4 (`Error`) → 0 (`Reset`) → 1 ... without end. The engine stops the search after 256 evolutions in each cycle and warns once in the console (*"The GRAFCET does not reach a stable situation after 256 evolutions: is there a loop of transitions always true?"*). Run by the engine, three cycles give:

  ```text
  cycle 0: 256 firings, steps X1  outputs: V1+
  cycle 1: 256 firings, steps X4  outputs: AlarmLamp Buzzer
  cycle 2: 256 firings, steps X0  outputs:
  ```

The step where the search stops changes at each cycle (256 is not a multiple of 3), so the outputs flicker, and the editor does 256 firings per frame: it uses a whole core of the processor. A real controller would do the same. Make one receptivity of each loop wait for something else: an edge (`↑Start`), the opposite level (`/Start` on the way back), or a time (`1s/X4`).
- **A level where an edge is needed.** `Ack` on two consecutive transitions: one press goes through both. `↑Ack` fires one transition per press.
- **Transient steps and stored actions.** The editor computes the actions from the stable situation only: a step crossed during the search of the stability runs none of its actions, not even an `S` or a `P1`. Do not count on a transient step to set an output.
- **Forcing is read at the beginning of the cycle.** When the emergency stop activates the safety step, the forced graphs take their situation at the next cycle: one cycle later (see the run of [chapter 10](10-GEMMA.md)).
- **Step numbers.** `X3` is the step whose *identifier* is 3 (the big number in the square), not its caption. When you delete a step, the last step takes its number; the receptivities of all the graphs (`X5`, `Production:X5`) follow the new number, and its actions go with it.

## 13. Going further

- The standard: IEC 60848:2013, *GRAFCET specification language for sequential function charts*.
- French lectures of the [bibliography](Bibliography.md): the GRAFCET course of O. Kamach, the ENSIAME course of J.-C. Popieul, the videos *Programmation d'un Grafcet en C (pour Arduino)*.
- The control panel and its JSON: [chapter 15](15-File-Formats.md); the engine in your own program: [chapter 16](16-Library.md); the C++ code for a board and its debugging: [chapters 12](12-Code-Generation.md) and [13](13-Debugging.md).

---

Next: [10. GEMMA](10-GEMMA.md) — the start and stop modes of a machine, with several GRAFCETs that force each other.
