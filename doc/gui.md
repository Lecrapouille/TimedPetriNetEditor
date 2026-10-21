# The graphical interface

## Windows

- **Nets** in the middle: one window per net of the document (a GEMMA has
  three, shown side by side; they keep their share of the width when the main
  window changes size). A net is fitted to its window until you move or zoom
  its view. Its type and its description are written in its top left corner.
- **Inspector** on the right: the **Document** (its description, the
  description of each net, the GRAFCET inputs and outputs), places (steps),
  transitions, arcs and GRAFCET actions; under it the GRAFCET **Control
  panel** and the **Analysis** window of timed event graphs.
- **Console** and the GRAFCET **Timing diagram** at the bottom, and the
  GRAFCET **Outputs** next to them.
- **Notifications**: the last messages appear a few seconds in the bottom
  right corner of the nets; all of them stay in the console.
- **Status bar**: what the mouse buttons would do now, the counts of nodes,
  the position of the mouse and the zoom.

## Console

One button per kind of message, with its count: *Info*, *Warnings*,
*Errors*, *Simulation* (the events of the simulation). A click hides or shows
them. The filter keeps the messages containing a text; *Copy* puts the shown
messages in the clipboard. The same message given several times in a row is
written once, with its count `(x3)`.

The *Simulation* messages use the words of the Gantt chart:

```
t = 4.00  M1 starts P1 (k = 2), until 7.00          a flowshop operation
t = 1.50  T3 (Ready) fires (k = 1), until 3.50      another transition
t = 2.20  Production: T1 [30s/X1]  X1 -> X2         a GRAFCET transition
t = 2.20  input Dcy = 1                             a GRAFCET input
```

The layout and the theme are saved in the settings folder of the user
(`~/.config/TimedPetriNetEditor` on Linux, `~/Library/Application
Support/TimedPetriNetEditor` on macOS, `%APPDATA%\TimedPetriNetEditor` on
Windows). *View > Reset the layout of the windows* builds the default one again.

## Mouse

There is no mode to choose. What a button does depends on what is under the
mouse, and on whether the mouse moves before the button is released (a move
of less than 4 pixels is a click). The left button draws (places, arcs,
selection), the right button moves (nodes, view). The status bar always says
what the buttons would do.

| Under the mouse | Click | Drag |
|-----------------|-------|------|
| nothing, left button | add a place (in a timed event graph: a transition); with a selection, the first click only clears it | select in a rectangle |
| nothing, right button | add a transition | move the view |
| node, left button | select it (Ctrl: add or remove it) | draw an arc from it |
| node, right button | menu of the node | move it, with the selection |
| arc, left button | select it | select in a rectangle |
| arc, right button | menu of the arc | move the view |
| middle button | | move the view |

- An arc released in the void creates a node of the other kind there; between
  two nodes of the same kind, a node is inserted in the middle.
- Double click: rename a node (for a GRAFCET transition: its receptivity,
  checked while typing); edit the duration of an arc.
- Wheel: zoom around the mouse. Shift + wheel, or a touchpad: move the view.
- While simulating a Petri net, a left click on a transition fires it.

## Keys

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
| Ctrl+Z, Ctrl+Y (Ctrl+Shift+Z) | undo, redo (moves too) |
| Ctrl+C, Ctrl+X, Ctrl+V, Ctrl+A | copy, cut, paste at the mouse, select all |
| Esc | cancel the current drag, else clear the selection |
| Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Q | new, open, save, save as, quit |
| Ctrl+=, Ctrl+-, Ctrl+0 | zoom in, zoom out, fit |
| F7 | GRAFCET: apply the inputs changed together |
| F1 | help |

## Simulation

*Run* / *Stop* (Space), *Pause* (P) and the speed (x0.1 to x50) are in the
toolbar and in the *Simulation* menu: at x10, a GRAFCET timer `30s/X1` lasts
3 s, and the tokens of a timed net move 10 times faster. The top left corner
of each net writes the simulated time. Several nets of a document: a click in
a net makes it the active one (the inspector shows it) and acts at once.

## Display

*View* (and the check boxes of the inspector) shows or hides the
identifiers of the places and transitions (`P0`, `T1`, the numbers of the
GRAFCET steps, `(1)` on the left of a GRAFCET transition), and the boxes of
the GRAFCET actions: hidden, a step shows a small tag with the number of its
actions (green when one runs), for a compact net.

## GRAFCET

- **Actions**: choosing a qualifier shows what it does and its timing
  diagram (*Help > GRAFCET actions* shows them all). A conditional action is
  drawn `if Dr: Motor`; a forcing order in a double frame,
  `Production{init}`.
- **Transitions**: *Syntax help* opens the table of what to write in a
  receptivity. While simulating, the timers that count (`30s/X1`, the delay
  of the transition) are drawn under the receptivity, `12.3 / 30 s`, and in
  the Transitions panel; the Steps panel writes how long each active step is
  active, the Actions panel how far the timed actions (D, L, SD, DS, SL) are.
- **Timing diagram**: the active steps, the inputs and the outputs over time,
  one row each (steps blue, inputs orange, outputs green, a curve for the
  sensor values). It shows the last seconds of the simulation (*Follow*), or
  any span with the time line; the mouse gives the time and the value.
- Removing a step gives its number to the last step (the numbers stay
  consecutive): its actions go with it, and the receptivities of all the
  graphs that read it (`X3`, `Production:X3`) follow its new number. What
  read the removed step reads a missing step (an error at the start).
- **Control panel**: the inputs drawn as the desk of a machine: emergency
  stop, push buttons, pulses, switches, selectors (exclusive positions such
  as Auto / Manu), sensor values (with a gauge), clocks. While stopped, a
  click changes the initial value saved with the document; while simulating,
  it acts at once (or at the next *Apply (F7)* with *Apply together*). A
  right click keeps a push button pressed. *Fit* draws the buttons smaller to
  see all of them; each section can be folded. *Edit* chooses the kind,
  color and description of each input and output (inputs and outputs not
  described are not saved, those no net uses any more are forgotten). See
  doc/save.md for the JSON.
- **Outputs**: the actuators named by the actions: pilot lights, motors
  (their fan turns), heaters (they glow), valves, horns.

## Menus

- **Edit > Arrange top-down** places the net in layers from its initial
  steps, the way a GRAFCET is drawn.
- **Analysis** (timed event graphs): critical circuit and cycle time, Gantt
  chart of the earliest dates, (max,+) linear system, dater and counter
  equations, adjacency matrices, firing dates of the simulation. Matrices
  are tables; *Copy as Julia* puts one in the clipboard.

## Gantt chart

Each transition followed by a duration is an operation: its bar starts at a
firing of the transition and lasts the duration of its output arc. A
flowshop transition `M1_P1` (piece `P1` on machine `M1`) is drawn on the row
`M1` with the color of `P1`; other transitions get their own row. The label
`P1 k=2` means the second time (cycle k = 2) the operation is done (the same
k as in the dater equations `T1(k) = max(1 + T0(k), 2 + T2(k - 1))`). The dates
are the earliest ones of the (max,+) dater equation
`x(k) = max(x_i(k - tokens_i) + duration_i)` over the input places, the
initial tokens being available at 0. While simulating, a red line shows the
time of the simulation, and the console writes the same events. Pause (P) and
a lower speed help to follow them.

## Command line

```
TimedPetriNetEditor [-r] [-s|-w screenshot.png] [petri.json]
```

`-r` starts the simulation once the file is loaded, `-s` saves the net (`-w`
the whole window) as a PNG file once drawn, then quits.
