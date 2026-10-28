# ⚖️ Conflict

One coin, two drinks: a choice.

![Conflict](Conflict.png)

## What it models

A coin can buy a tea or a coffee, not both. `Tea` and `Coffee` share their input place `Coin`.

## What it teaches

- [Conflict (choice)](../../../doc/en/03-Petri-Nets.md#52-conflict-a-choice)
- [Who resolves a conflict in the editor](../../../doc/en/03-Petri-Nets.md#52-conflict-a-choice)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/Conflict/Conflict.json
```

1. Press **Space**: `Tea` and `Coffee` are both green.
2. Fire `Tea`: `Coffee` turns grey. The choice is made, the other branch is lost.
3. Stop (**Space**), run again and choose `Coffee`.
4. *Analysis > Properties*: the net ends in a deadlock after either choice, and has no T-invariant: nothing comes back.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
