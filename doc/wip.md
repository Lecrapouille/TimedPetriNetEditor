# Work in progress

- The whole GUI is currently in refacto. It will change a lot in next commits.
- Time duration are not yet correctly editable from the net. Use the menu instead.
- Showing critical cycles for graph events does not support having transitions
  without predecessors (inputs). For example this Petri net: `T0 -> P0 -> T1`.
  *Workaround:* Either modify your net either by removing your inputs or making
  inputs cycling to themselves with a `-inf` time (which makes duality issues).
- Not all error messages are displayed on the GUI. Read the logs on your Unix
  console.
- We cannot change the color of nodes and arcs. We cannot merge several nodes
  into a sub-net for simplifying the net. Cannot delete arcs. Cannot make
  undo/redo actions.
- Use bezier to draw arcs. Text and arrows are mixed together.
- Julia API: missing add/remove arcs.
- Code for some export/import files are here but not yet done.
