# Description of the file format used for saving Petri net

JSON file format has been chosen for saving Petri net. This was initially made
to be compatible with [pnet-simulator](https://github.com/igorakim/pnet-simulator)
but the format is no longer compatible. Here, is an example of its content:

```json
{
  "revision": 1,
  "type": "Timed Petri net",
  "description": "What the document is about (optional)",
  "nets": [
    {
       "name": "Hello World",
       "description": "What this net does (optional)",
       "places": [
            { "id": 0, "caption": "P0", "tokens": 1, "x": 184, "y": 295 },
            { "id": 1, "caption": "P1", "tokens": 0, "x": 513, "y": 295 }
       ],
       "transitions": [
            { "id": 0, "caption": "T0", "x": 351, "y": 295 }
       ],
       "arcs": [
            { "from": "P0", "to": "T0" },
            { "from": "T0", "to": "P1", "duration": 3 },
            { "from": "T0", "to": "P0", "duration": 1 }
       ]
    }
  ]
}
```

The JSON file has a revision number for its syntax (currently 1, with the
descriptions and the GRAFCET inputs and outputs; older files, up to the
revision 5 of the previous numbering, are still read, including their GRAFCET
receptivities in reverse polish notation). A document holds one or several
nets (`"nets": [ { ... }, { ... } ]`), all of the same type.

The optional `description` of the document (after `type`) and of each net
(after `name`) explain them to the reader: the editor shows the first in the
Document window, the others in the top left corner of each net.

A Petri net is composed of three arrays (the `[ ]`): `Places`, `Transitions` and
`Arcs`. In this example, is stored in the JSON file a Petri net made of two
places, one transition, and two arcs.

Places and Transitions:
- their unique identifier (unsigned int) i.e. `0`, `1`. All ids shall be
  consecutive (no holes).
- their caption is a string with spaces accepted.
- their X and Y coordinate (float) in the screen i.e. `T0` is placed at
  `(351,295)`.
- for places only: the number of tokens they hold (zero or positive number)
  i.e. `P1` has 0 tokens while `P0` has a single token.
- for transition only: the angle (in degree) of rotation when displayed.

Arcs:
- re directed links between two nodes of the different types (i.e. the first
  arc in this example, links the origin place `P0` to the destination transition `T0`).
  Therefore an arc cannot link two places or link two transitions.
- have no identifiers because the pair origin and destination nodes make them unique.
- has a unit of time (positive value) i.e. the arc `T0 --> P1` has 3 units of
  times (float). This time is only used for arc `Transition` to `Place` this
  means that for arc `Place` to `Transition` this value is not used.

Net:
- Have a name (here "Hello World") and is typed of "Petri net", "Timed Petri
  net", "Timed event graph" or "GRAFCET".

## GRAFCET

A GRAFCET net has, after its arcs, the actions of its steps:

```json
"actions": [
  { "place_id": 1, "qualifier": "N", "color": "green", "name": "Motor",
    "script": "comment", "condition": "Dr" },
  { "place_id": 2, "qualifier": "D", "name": "Lamp", "duration": 3 },
  { "place_id": 0, "forcing": ["Production:{init}", "Control:{init}"] }
]
```

`qualifier` is one of `N S R D L SD DS SL P1 P0`, `duration` is the d of the
timed qualifiers, `condition` makes a conditional action (drawn `if Dr:
Motor`), `forcing` lists forcing orders (drawn `Production{init}`; the colon is
optional). An action with forcing orders only has no name.

After the nets, the document lists the inputs of the GRAFCET (the receptivities
read them) and the outputs (the names of the actions). Their `kind` says how
the control panel draws them:

```json
"inputs": [
  { "name": "Au", "kind": "emergency", "description": "Emergency stop", "initial": 0 },
  { "name": "Dcy", "kind": "button", "description": "Start", "initial": 0, "color": "green" },
  { "name": "Auto", "kind": "selector", "group": "Mode", "initial": 1 },
  { "name": "Manu", "kind": "selector", "group": "Mode", "initial": 0 },
  { "name": "Temperature", "kind": "value", "initial": 20, "min": 20, "max": 120,
    "unit": "°C", "driven_by": "Oven", "rise": 6, "fall": 1 },
  { "name": "Blink", "kind": "clock", "initial": 0, "period": 1 }
],
"outputs": [
  { "name": "Motor", "kind": "motor", "description": "Input conveyor" },
  { "name": "Oven", "kind": "heater" }
]
```

| Input kind | Behavior |
|------------|----------|
| `switch` (default) | a click turns it on or off, and it stays |
| `button` | push button: on while pressed (one cycle at least) |
| `pulse` | on during one cycle of the GRAFCET per click |
| `emergency` | emergency stop: pushed (on) until pulled back |
| `selector` | a position of the selector `group`: one input of the group only is on |
| `value` | integer value of a sensor between `min` and `max`, read with `[Temperature >= 80]`; with `driven_by`, it goes up by `rise` per second while the output is on, down by `fall` else |
| `clock` | on during half of its `period` (seconds), then off |

Output kinds: `light` (default, the color of its actions), `motor`, `heater`,
`valve`, `horn`. `color` of a button: `green`, `red`, `black`, `yellow`,
`blue`, `white`, `orange`.
