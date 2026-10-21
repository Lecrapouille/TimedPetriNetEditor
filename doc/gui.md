# The graphical interface

## Windows

- **Nets** in the middle: one tab per net of the document (a GEMMA has three,
  shown side by side).
- **Inspector** on the right: places (steps), transitions, arcs and GRAFCET
  actions; under it the GRAFCET inputs and outputs, and the **Analysis**
  window of timed event graphs.
- **Console** and **Message** at the bottom.
- **Status bar**: what the mouse buttons would do now, the counts of nodes,
  the position of the mouse and the zoom.

The layout and the theme are saved in the settings folder of the user
(`~/.config/TimedPetriNetEditor` on Linux, `~/Library/Application
Support/TimedPetriNetEditor` on macOS, `%APPDATA%\TimedPetriNetEditor` on
Windows). *View > Reset the layout of the windows* builds the default one again.

## Mouse

There is no mode to choose. What a button does depends on what is under the
mouse, and on whether the mouse moves before the button is released (a move
of less than 4 pixels is a click). The status bar always says what the
buttons would do.

| Under the mouse | Click | Drag |
|-----------------|-------|------|
| nothing, left button | add a place (in a timed event graph: a transition); with a selection, the first click only clears it | select in a rectangle |
| nothing, right button | add a transition | move the view |
| node, left button | select it (Ctrl: add or remove it) | move it, with the selection |
| node, right button | menu of the node | draw an arc from it |
| arc, left button | select it | select in a rectangle |
| middle button, or Space held | | move the view |

- An arc released in the void creates a node of the other kind there; between
  two nodes of the same kind, a node is inserted in the middle.
- Double click: rename a node (for a GRAFCET transition: its receptivity,
  checked while typing); edit the duration of an arc.
- Wheel: zoom around the mouse. Shift + wheel, or a touchpad: move the view.
- While simulating a Petri net, a click on a transition fires it.

## Keys

| Key | Action |
|-----|--------|
| F | fit the net in its window |
| G | snap the nodes to the grid |
| L | spring layout (again to stop) |
| + / - | tokens of the place under the mouse |
| Delete, Backspace | delete the selection, else what is under the mouse |
| Arrows (Shift: by 1) | move the selection |
| Ctrl+Z, Ctrl+Y (Ctrl+Shift+Z) | undo, redo (moves too) |
| Ctrl+C, Ctrl+X, Ctrl+V, Ctrl+A | copy, cut, paste at the mouse, select all |
| R, F5 | start or stop the simulation |
| Esc | cancel the current drag, else clear the selection |
| Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Q | new, open, save, save as, quit |
| Ctrl+=, Ctrl+-, Ctrl+0 | zoom in, zoom out, fit |

## Menus

- **Edit > Arrange top-down** places the net in layers from its initial
  steps, the way a GRAFCET is drawn.
- **Analysis** (timed event graphs): critical circuit and cycle time, Gantt
  chart of the earliest dates, (max,+) linear system, dater and counter
  equations, adjacency matrices, firing dates of the simulation. Matrices
  are tables; *Copy as Julia* puts one in the clipboard.
