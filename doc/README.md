# TimedPetriNetEditor documentation

Read the chapters in order: each one uses the words of the previous ones.
Chapters 1 to 4 are for everybody, even if you never saw a Petri net.
Chapters 5 to 10 are references: open them when you need them.

## Learn

| # | Chapter | You will learn | Time |
|---|---------|----------------|------|
| 1 | [Introduction](01-Introduction.md) | What places, transitions, tokens are; the four kinds of nets of the editor | 10 min |
| 2 | [Getting started](02-Getting-Started.md) | Build, open an example, draw your first net with the mouse, run it | 15 min |
| 3 | [Timed nets and (max,+) algebra](03-Timed-Nets-and-Max-Plus.md) | Durations, timed event graphs, cycle time, Gantt chart, dater equations | 30 min |
| 4 | [GRAFCET and GEMMA](04-GRAFCET-and-GEMMA.md) | Steps, receptivities, actions, the control panel, the timing diagram; a dryer with three graphs | 30 min |

## Reference

| # | Chapter | Contents |
|---|---------|----------|
| 5 | [Exports and code generation](05-Exports-and-Code-Generation.md) | SVG, LaTeX, Graphviz, PNML, GRML, journals; C++ for the runtime and for GRAFCETs; the editor as a debugger |
| 6 | [File format](06-File-Format.md) | The JSON file: nets, GRAFCET actions, inputs and outputs, descriptions |
| 7 | [Editor reference](07-Editor-Reference.md) | Every window, mouse gesture, key, menu and command line option |
| 8 | [Julia](08-Julia.md) | Drive the editor and the (max,+) algorithms from Julia |
| 9 | [Remote control](09-Remote-Control.md) | JSON commands over ZeroMQ: load, start, fire, read the state; follow a running program |
| 10 | [Use the library in C++](10-Library.md) | Load a file, simulate, run a GRAFCET, compute dates, run your functions, check the properties, without the editor |
| 11 | [From the editor to the code](11-From-Editor-To-Code.md) | A robot from the drawing to a running program, watched in the editor |
| — | [Glossary](Glossary.md) | The words of the editor in simple English |
| — | [Bibliography](Bibliography.md) | Books, lectures and other editors |

## Quick paths

- *"I want to see a net run now."* → [2. Getting started](02-Getting-Started.md)
- *"How fast can my factory produce?"* → [3. Timed nets](03-Timed-Nets-and-Max-Plus.md)
- *"I teach or learn automation (GRAFCET, GEMMA)."* → [4. GRAFCET and GEMMA](04-GRAFCET-and-GEMMA.md)
- *"Which key does what?"* → [7. Editor reference](07-Editor-Reference.md)
- *"I want my program to use the nets."* → [11. From the editor to the code](11-From-Editor-To-Code.md), [10. Use the library](10-Library.md), [9. Remote control](09-Remote-Control.md)
- *"Can my net deadlock?"* → [7. Editor reference](07-Editor-Reference.md#7-journal-and-analysis)

## A note on the language

This documentation is written in simple English, with short sentences and
common words, so that readers whose first language is not English (French
readers in particular) can follow it easily. Every output shown was produced
by the editor or by the library, not invented.
