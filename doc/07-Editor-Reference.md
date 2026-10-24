# 7. Editor reference

What you will find here: every window, mouse gesture, key, menu and command
line option of the editor. The tutorials are in chapters
[2](02-Getting-Started.md), [3](03-Timed-Nets-and-Max-Plus.md) and
[4](04-GRAFCET-and-GEMMA.md). In the editor, `F1` opens the same help.

---

## 1. Windows

| Window | Shows | When |
|--------|-------|------|
| Nets (middle) | one window per net of the document, side by side; the type of the net, its description and, while simulating, the time, the speed and *paused* | always |
| Document | description of the file and of each net (*Edit* to change them); GRAFCET inputs and outputs | always |
| Places / Steps | captions and tokens; for a running GRAFCET, how long each active step is active | always |
| Transitions | captions or receptivities; GRAFCET: delays, syntax errors, *Syntax help*, timers that count | always |
| Arcs | kind, weight, color of the tokens, duration of each arc (timed event graph: durations) | Petri and timed nets |
| Actions | actions of each step: qualifier with its timing diagram, name, condition, duration, forcing orders | GRAFCET |
| Control panel | the inputs as the buttons of a machine | GRAFCET |
| Outputs | the actuators: lights, motors, heaters, valves, horns | GRAFCET |
| Timing diagram | steps, inputs and outputs over time | GRAFCET |
| Analysis | *Properties* of any net: deadlocks, dead transitions, bounds, liveness, invariants (§7); event graphs: critical circuit, Gantt chart, linear system, equations, matrices, firing dates ([chapter 3](03-Timed-Nets-and-Max-Plus.md#5-reference-the-analysis-window)) | always |
| Console | all the messages (§6) | always |
| Journal | the firings of the simulation, of a journal opened, or of a connected program; replays them on the net (§7) | always |
| Program | the program the editor follows: its inputs and outputs, its transitions without function ([chapter 9](09-Remote-Control.md#4-follow-a-running-program)) | connected |
| Status bar | what the mouse buttons would do now; counts of nodes, position, zoom | always |

A net is fitted to its window, again each time the window changes size,
until you move or zoom its view (`F` fits it again). The windows of the nets
keep their share of the width when the main window changes size. The layout
and the theme are saved in the settings folder of the user
(`~/.config/TimedPetriNetEditor` on Linux, `~/Library/Application
Support/TimedPetriNetEditor` on macOS, `%APPDATA%\TimedPetriNetEditor` on
Windows). *View > Reset the layout of the windows* builds the default one.

## 2. Mouse

There is no mode to choose. What a button does depends on what is under the
mouse, and on whether the mouse moves before the button is released (less
than 4 pixels is a click). **The left button draws, the right button moves.**

| Under the mouse | Click | Drag |
|-----------------|-------|------|
| nothing, left | add a place (timed event graph: a transition); with a selection, the first click only clears it | select in a rectangle |
| nothing, right | add a transition | move the view |
| node, left | select it (Ctrl: add or remove it) | draw an arc from it |
| node, right | menu of the node (place: tokens, max tokens) | move it, with the selection |
| arc, left | select it | select in a rectangle |
| arc, right | menu of the arc: duration and weight, kind (normal, inhibitor, reset), color of the tokens | move the view |
| middle button | | move the view |

- An arc released in the void creates a node of the other kind there;
  between two nodes of the same kind, a node is inserted in the middle.
- Double click: rename a node (a GRAFCET transition: its receptivity,
  checked while typing); an arc: its duration (timed nets) and its weight,
  and the tokens of its place in a timed event graph (Enter applies, Esc
  cancels).
- Nodes with a halo: marked by the *Journal* (the transitions whose function
  runs), a connected program (no function) or the *Analysis* (dead, not
  live, unbounded, the nodes of an invariant).
- Wheel: zoom around the mouse. Shift + wheel, or a touchpad: move the view.
- While simulating a Petri net, a left click on a transition fires it.
- With several nets, a click in a net makes it the active one (the
  inspector shows it) and acts at once.

## 3. Keys

| Key | Action |
|-----|--------|
| Space | start or stop the simulation |
| P | pause or resume the simulation |
| F | fit the net in its window |
| G | snap the nodes to the grid |
| L | spring layout (again to stop) |
| + / - | tokens of the place under the mouse |
| Delete, Backspace | delete the selection, else what is under the mouse |
| Arrows (Shift: by 1) | move the selection |
| Esc | cancel the current drag, else clear the selection |
| Ctrl+Z, Ctrl+Y (Ctrl+Shift+Z) | undo, redo |
| Ctrl+C, Ctrl+X, Ctrl+V, Ctrl+A | copy, cut, paste at the mouse, select all |
| Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Q | clear, open, save, save as, quit |
| Ctrl+=, Ctrl+-, Ctrl+0 | zoom in, zoom out, fit |
| F7 | GRAFCET: apply the inputs changed together |
| F1 | help |

## 4. Simulation

| Control | Where | Effect |
|---------|-------|--------|
| Run / Stop | toolbar, *Simulation* menu, Space | the net comes back to its first marking when stopped |
| Pause / Resume | toolbar, *Simulation* menu, P | the simulation does not evolve |
| Speed x0.1 ... x50 | toolbar, *Simulation > Speed* | at x10, `30s/X1` lasts 3 s and the tokens move 10 times faster |

Opening, saving or exporting a file while simulating first stops the
simulation (a message says it).

## 5. Display

*View* and the check boxes of the inspector:

| Option | Effect |
|--------|--------|
| Place identifiers | `P0`, `P1` next to the captions; GRAFCET: the numbers of the steps (else the caption alone) |
| Transition identifiers | `T0` next to the captions; GRAFCET: `(1)` on the left of the transitions |
| GRAFCET action boxes | hidden: a step shows a small tag with the number of its actions (green when one runs), for a compact net |
| Grid, Snap (G) | the grid; nodes moved snap to it |
| Theme | dark, light, classic |

## 6. Console and notifications

One button per kind of message, with its count: *Info*, *Warnings*,
*Errors*, *Simulation* (the GRAFCET inputs changed while running; the
firings are in the *Journal*). A click hides or shows them. The filter keeps the
messages containing a text; *Copy* puts the shown messages in the
clipboard. The same message given several times in a row is written once,
with its count `(x3)`. The last messages also appear a few seconds in the
bottom right corner of the nets.

## 7. Journal and analysis

The **Journal** lists the firings: date, instance (the net, or an instance
of a program), transition, its number k, the event (`Scheduled`: tokens
taken, `Started`: its function runs, `Completed`: tokens given) and the
result (the color of the tokens given). It holds the firings of the last
simulation, a journal opened (*Open*: JSON lines of the runtime), the path
to a deadlock found by the analysis, or what a connected program publishes.
*Save* writes it as JSON lines, CSV or XES (process mining). *Replay* shows
the marking of the net at a date: the slider, `<` and `>` go from one event
to the next, *Play* runs the time; the transitions whose function runs are
marked. A click on a line goes to its date.

The tab **Properties** of the *Analysis* window, for every net (the
durations and the GRAFCET rules not considered): *Compute* explores the
markings (small nets: again at each change, up to *max markings*). The
summary says whether the net can deadlock, has dead transitions, is bounded
and live; the deadlock has its marking and the shortest path to it
(*Replay the path in the Journal*); the bounds of each place; the
P-invariants (the weighted tokens of these places never change) and the
T-invariants (firing these transitions gives the marking back): a click
marks their nodes. *Show on the net* marks the dead transitions, the ones
not live and the unbounded places. *Graph of the markings* saves it for
Graphviz. The same analyses run in a terminal or a CI:
`tpne-check [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json`
(exit code 1 when a net can deadlock).

## 8. GRAFCET windows

- **Control panel**: one widget per input, by kind
  ([chapter 4](04-GRAFCET-and-GEMMA.md#6-reference-the-control-panel)).
  While stopped, a click changes the initial value (saved); while running,
  it acts at once, or at *Apply (F7)* with *Apply together*. A right click
  keeps a push button pressed. *Fit* draws the buttons smaller to see all of
  them; each section folds. *Edit* chooses the kind, color and description
  of each input and output.
- **Outputs**: the actuators named by the actions, on or off.
- **Timing diagram**: *Steps*, *Inputs*, *Outputs* choose the rows; *Only
  what changed* hides the steps never active and the signals always 0; the
  slider gives the span shown; *Follow* shows the last seconds, else a time
  line chooses the span. The mouse gives the time and the value.
- **Actions**: a qualifier shows its meaning and its timing diagram; `(?)`
  opens all of them.
- Removing a step gives its number to the last step: its actions go with it,
  and the receptivities of all the graphs (`X3`, `Production:X3`) follow.
  What read the removed step reads a missing step (an error at the start).

## 9. Menus

| Menu | Holds |
|------|-------|
| File | new (Petri net, timed Petri net, timed event graph, GRAFCET, GEMMA with 3 GRAFCETs), clear, open, import, save, save as, export, screenshot, documents, quit |
| Edit | undo, redo, type of net, add a net to the document, to canonical form, arrange top-down (layers from the initial steps, as a GRAFCET is drawn), spring layout, align to the grid, clear the net |
| View | fit, zoom, grid, snap, identifiers, action boxes, theme, reset the layout, remote control |
| Simulation | start/stop, pause, speed, the nets of the document |
| Analysis | the properties of the net; the (max,+) tabs of the Analysis window (event graphs) |
| Debug | connect to a program publishing its journal, open a manifest (its transitions without function), open and save a journal, clear the marks of the nodes |
| Help | mouse and keys, GRAFCET receptivities, GRAFCET actions, GRAFCET control panel, about |

## 10. Command line

```text
TimedPetriNetEditor [-r] [-s|-w screenshot.png] [-d seconds] [-f window] [-c endpoint] [petri.json]
```

| Option | Effect |
|--------|--------|
| `petri.json` | the file to open (also `.flowshop`, `.pnml`, `.grml`, `.teg`) |
| `-r` | start the simulation once the file is loaded |
| `-s file.png` | save the net as a PNG file once drawn, then quit |
| `-w file.png` | the same for the whole window |
| `-d seconds` | wait before the screenshot (the simulation runs meanwhile) |
| `-f window` | bring this window in front: `Journal`, `Analysis` (its properties), `Console` ... |
| `-c endpoint` | follow the program publishing its journal there (`tcp://localhost:5556`) |

The pictures of this documentation were made with `-w`.

---

Next: [8. Julia](08-Julia.md).
