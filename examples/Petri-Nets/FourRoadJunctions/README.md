# 🛣️ Four road junctions

Cars entering and leaving a network of four junctions.

![Four road junctions](FourRoadJunctions.png)

## What it models

The source transitions `u0` to `u3` (no input place) let cars in; the sink transitions `y0` to `y3` (no output place) let them out.

## What it teaches

- [Sources and sinks](../../../doc/03-Petri-Nets.md#3-the-firing-rule)
- [State explosion](../../../doc/04-Analysis.md#23-the-limit-state-explosion)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/FourRoadJunctions/FourRoadJunctions.json
```

1. Press **Space** and fire the `u` transitions: the cars spread in the network.
2. *Analysis > Properties*: the exploration stops after its maximum number of markings; with sources, the reachable markings never end.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
