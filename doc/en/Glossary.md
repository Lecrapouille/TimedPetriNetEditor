# Glossary


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/Glossary.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/Glossary.md)

The words of TimedPetriNetEditor in simple English. Chapter 1 explains the first ones with pictures.

| Word | Meaning |
|------|---------|
| **Petri net** | A drawing of a system made of places, transitions and arcs, where tokens move. |
| **Place** | A circle: a state or a stock. It holds tokens. Named `P0`, `P1` ... |
| **Token** | A black dot in a place: a resource, a piece, a person, "this state is true". |
| **Transition** | A bar: an event. Named `T0`, `T1` ... |
| **Arc** | An arrow from a place to a transition or from a transition to a place. |
| **Enabled** | A transition is enabled when every place before it has a token. |
| **Fire** | An enabled transition fires: it takes one token from each place before it and puts one in each place after it. |
| **Marking** | How many tokens each place holds: the state of the whole net. |
| **Identifier** | The name of a node given by the editor (`P3`, `T1`); the **caption** is the text you write. |
| **Duration** | A time written on an arc going out of a transition: the tokens take this time to arrive. |
| **Timed Petri net** | A Petri net with durations. |
| **Event graph** | A Petri net where each place has exactly one arc in and one arc out: no choice, only synchronization. |
| **Timed event graph** | An event graph with durations. The editor draws it without its places (they are on the arcs). |
| **(max,+) algebra** | An algebra where "plus" is `max` and "times" is `+`. It turns the dates of a timed event graph into linear equations. |
| **ε (epsilon)** | The "zero" of the (max,+) algebra, `-∞`: there is no arc. |
| **Dater equation** | `x(k)`: the date of the k-th firing of a transition, computed from the dates of the transitions before it. |
| **Counter equation** | `x(t)`: how many times a transition fired before the time t. |
| **k (cycle)** | The number of a firing: k = 2 is the second time the transition fires. |
| **Cycle time (λ)** | In the long run, the time between two firings of a transition of a timed event graph: the speed of the system. |
| **Critical circuit** | The loop of the net that sets the cycle time: the bottleneck. |
| **Flowshop** | A factory that produces parts. On its timed event graph, each machine is a horizontal cycle and each conveyor a vertical one. |
| **Gantt chart** | One row per machine, one bar per operation and cycle. |
| **GRAFCET** | A French standard (IEC 60848, also called SFC) to describe the control of a machine: steps, transitions with conditions, actions. |
| **Step** | The place of a GRAFCET: active (it has a token) or not. Named `X0`, `X1` ... |
| **Initial step** | A step active at the start, drawn with a double square. |
| **Receptivity** | The condition written next to a GRAFCET transition, like `Dcy . /Au`. |
| **Action** | What an active step does: switch on a motor, a lamp ... |
| **Qualifier** | The letter of an action: `N` (while the step is active), `S` (set), `R` (reset), `D` (delayed) ... |
| **Input** | What the GRAFCET reads: a button, a sensor. |
| **Output** | What the GRAFCET drives: a motor, a heater, a lamp. It has the name of its actions. |
| **Edge** | `↑a`: a has just become true (rising edge); `↓a`: it has just become false. |
| **Forcing order** | A step of one graph puts another graph in a given situation: `Production{init}`. |
| **GEMMA** | A French method (Guide d'Étude des Modes de Marche et d'Arrêt) to describe how a machine starts, stops and recovers. Often three graphs: safety, control, production. |
| **GMMA** | Graph of the run and stop modes (*Graphe des Modes de Marche et d'Arrêt*): the procedures of the GEMMA a machine keeps and the links between them. In the editor, the GRAFCET Control made from the GEMMA window. |
| **Control panel** | The window of the editor that shows the inputs of a GRAFCET as the buttons of a machine. |
| **Timing diagram** | The window that draws the steps, inputs and outputs over time. |
| **Document** | A file: one or several nets of the same type, their description, the inputs and outputs. |
