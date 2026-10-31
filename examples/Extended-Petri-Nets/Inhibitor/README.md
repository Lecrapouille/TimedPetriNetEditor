# 💡 Inhibitor arc

An inhibitor arc: switch off the light only when the room is empty.

![Inhibitor arc](Inhibitor.png)

## What it models

People go `Outside` ↔ `Inside`; they can enter only when the light is on (`Enter` reads `Light on`). `Switch off` has an inhibitor arc from `Inside` (the small circle): it fires only while `Inside` is empty.

## What it teaches

- [Inhibitor arcs](../../../doc/en/05-Extended-Petri-Nets.md#3-inhibitor-arcs)
- [Read arcs](../../../doc/en/03-Petri-Nets.md#6-patterns-of-modeling)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Extended-Petri-Nets/Inhibitor/Inhibitor.json
```

1. Press **Space**, fire `Enter`: `Switch off` turns grey.
2. Fire `Exit`: `Switch off` is enabled again; fire it, then `Switch on`.
3. *Analysis > Properties*: 5 markings; the note says the invariants ignore the inhibitor arc.

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/en/05-Extended-Petri-Nets.md)
