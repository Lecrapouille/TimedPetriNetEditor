# 6. File format

What you will find here: the JSON file written by *File > Save*, field by
field. You can write it by hand, or generate it from another program.

---

## 1. A small file

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

```text
   (●) P0 ──► ▭ T0 ──── 3 ────► ( ) P1
    ▲          │
    └─── 1 ────┘
```

## 2. The document

| Field | Meaning |
|-------|---------|
| `revision` | version of the syntax: 1. Older files (with a `revision` up to 5 of the old numbering) are still read; their GRAFCET receptivities in reverse polish notation (`a b .`) are converted. |
| `type` | `"Petri net"`, `"Timed Petri net"`, `"Timed event graph"` or `"GRAFCET"`: all the nets of the file have this type |
| `description` | optional: what the document is about, shown in the *Document* window |
| `nets` | one or several nets (a GEMMA has three) |
| `inputs`, `outputs` | GRAFCET only (§4) |

## 3. A net

| Field | Meaning |
|-------|---------|
| `name` | name of the net, also used by the other graphs (`Production:X3`, `Production{init}`) |
| `description` | optional: what the net does, written in its top left corner |
| `places` | `id` (0, 1, 2 ... consecutive, no hole), `caption` (any text), `tokens`, `x`, `y` |
| `transitions` | `id`, `caption` (the receptivity for a GRAFCET), `x`, `y`, `delay` (GRAFCET, seconds, optional) |
| `arcs` | `from`, `to`: `P<id>` or `T<id>`, a place to a transition or a transition to a place; `duration` on the arcs going out of a transition |
| `actions` | GRAFCET only (§4) |

An arc never links two places or two transitions, and has no identifier
(the pair `from`, `to` is unique).

## 4. GRAFCET: inputs and outputs

A GRAFCET net has, after its arcs, the actions of its steps:

```json
"actions": [
  { "place_id": 1, "qualifier": "N", "color": "green", "name": "Motor",
    "script": "comment", "condition": "Dr" },
  { "place_id": 2, "qualifier": "D", "name": "Lamp", "duration": 3 },
  { "place_id": 0, "forcing": ["Production:{init}", "Control:{init}"] }
]
```

| Field | Meaning |
|-------|---------|
| `place_id` | the step |
| `qualifier` | `N`, `S`, `R`, `D`, `L`, `SD`, `DS`, `SL`, `P1`, `P0` ([chapter 4](04-GRAFCET-and-GEMMA.md#5-reference-actions)) |
| `name` | the output; empty for an action with forcing orders only |
| `duration` | the d of the timed qualifiers, in seconds |
| `condition` | optional: the action runs only while it is true (drawn `if Dr: Motor`) |
| `color` | color of the pilot light: `green`, `red`, `orange`, `yellow`, `blue`, `white` |
| `script` | a comment (written in the generated code) |
| `forcing` | forcing orders, drawn `Production{init}` (the colon is optional) |

After the nets, the file lists the **inputs** of the GRAFCETs (what the
receptivities read) and their **outputs** (the names of the actions). The
`kind` says how the control panel draws them:

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

| Input field | Meaning |
|-------------|---------|
| `name` | as written in the receptivities |
| `kind` | `switch` (default), `button`, `pulse`, `emergency`, `selector`, `value`, `clock` ([chapter 4](04-GRAFCET-and-GEMMA.md#6-reference-the-control-panel)) |
| `description` | what it is, shown in the tooltip of the panel |
| `initial` | value at the start of the simulation |
| `color` | cap of a button or switch: `green`, `red`, `black`, `yellow`, `blue`, `white`, `orange` |
| `group` | `selector`: the inputs of the same group are the positions of one selector |
| `min`, `max`, `unit` | `value`: range and unit of the sensor |
| `driven_by`, `rise`, `fall` | `value`: goes up by `rise` per second while the output `driven_by` is on, down by `fall` else (a simple model of the machine) |
| `period` | `clock`: period in seconds |

| Output field | Meaning |
|--------------|---------|
| `name` | as written in the actions |
| `kind` | `light` (default), `motor`, `heater`, `valve`, `horn` |
| `description` | what it is |

The editor saves only the inputs and outputs you described (with *Edit* on
the control panel); the others are found again from the receptivities and
actions when the file is opened.

---

Next: [7. Editor reference](07-Editor-Reference.md).
