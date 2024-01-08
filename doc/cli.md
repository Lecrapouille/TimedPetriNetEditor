# Command line

The command line is optional. By default type of net is timed Petri. The type of
net is stored inside the file (GRAFCET, timed Petri net ...) but can be changed
(under condition for event graph) later from the editor menu.

```sh
TimedPetriNetEditor [petri.json]
```

Where:
- `[petri.json]` is an optional Petri net file to load (JSON format). The type
of net is stored inside the json file ((timed) Petri net, timed graph event,
GRAFCET ...) but the GUI allows you to swith of type.

Example:

```sh
TimedPetriNetEditor data/examples/Howard2.json
```
