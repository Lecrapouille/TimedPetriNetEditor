# Mouse and key bindings for the graphical interface

The editor does not offer any buttons, therefore all actions are directly made
from the mouse and the keyboard (like Emacs but without the Meta key ^^).
- `Escape` quit the application. The unsaved net will be asked for saving before the
  application closes.
- `Left mouse button pressed` on a caption or in duration (*work in progress*) to edit it.
  Press the Enter key to validate the content or press the Escape key to abort.
- `Left mouse button pressed`: add a new place. Note: this action is ignored if
  you are trying to create a node on an already existing node (place or
  transition).
- `Right mouse button pressed`: add a new transition. Note: this action is
  ignored if you are trying to create a node on an already existing node (place
  or transition).
- `Middle mouse button pressed`: start creating an arc either from an existing
  node (place or transition) as the origin or, if no node was selected by the mouse
  cursor, the editor will find the correct type depending on the destination
  node.
- `Middle mouse button released`: end creating an arc either from the selected
  node (place or transition) as the destination or, if no node is selected, the
  editor will find the correct type depending on the origin node. In the case
  you have tried to link two nodes of the same type (two places or two
  transitions) then an intermediate node of the opposite type will be created as
  well as an intermediate arc.
- `Middle mouse scroll`: zoom/unzoom the view.
- `Up`, `Down`, `Right`, `Left` keys: move the view.
- `L` key: same action as the middle mouse button.
- `M` key: move the node (place or transition) selected by the mouse cursor.
- Linux: `Delete` key: remove a node (place or transition). Note: since all identifiers
  shall be consecutive (no gaps), the unique identifier of the last created node
  of the same type will receive the unique identifier of the deleted one. Note:
  arcs cannot yet be deleted.
- MacOSX: `\` for deleting node.

The whole GUI is currently in refacto. The following commands will be removed in next
commits:
- `Z` key: clear the whole Petri net.
- Linux: `+`, `-` keys: add/remove a token on the place selected by the mouse cursor.
- MacOSX: `[`, `]` keys: add/remove a token on the place selected by the mouse cursor.
- `PageUp`, `PageDown` keys: rotate CW or CCW the transition selected by the
  mouse cursor.
- `R` or `SPACE`key: start (run) or stop the simulation. Note: if the simulation
  is stalled (no possible burning tokens), then the simulation will automatically
  stops and returns to the edition mode.
- `S` key: save the Petri net to a JSON file.
- `Shift S` key: save as the Petri net to a JSON file.
- `O` key: load a Petri net from a JSON file.
- `E` key: is the current timed Petri net a timed graph event?
- `C` key: show the first critical circuit if and only if the Petri net is a
  graph event.
- `F1` key: take a screenshot of the application.
- `H` key: show this help.
