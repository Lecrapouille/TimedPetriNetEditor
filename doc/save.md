# Description of the file format used for saving Petri net

JSON file format has been chosen for saving Petri net. This was initially made
to be compatible with [pnet-simulator](https://github.com/igorakim/pnet-simulator)
but the format is no longer compatible. Here, is an example of its content:

```json
{
  "revision": 3,
  "type": "Timed Petri net",
  "nets": [
    {
       "name": "Hello World",
       "places": [
            { "id": 0, "caption": "P0", "tokens": 1, "x": 184, "y": 295 },
            { "id": 1, "caption": "P1", "tokens": 0, "x": 513, "y": 295 }
       ],
       "transitions": [
            { "id": 0, "caption": "T0", "x": 351, "y": 295, "angle": 0 }
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

Json file has revison number concerning its syntax (currently version 3). Currently
only an array with a single net is loaded (`"nets": [ { ... ] ]`).

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
- Have a name (here "Hello World") and is typed of "Timed Petri net" or .
