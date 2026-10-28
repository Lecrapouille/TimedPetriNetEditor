# 🏠 Smart home safety

Four roller shutters, at most three of them down: a way out always stays open.

![Smart home safety](SmarthomeSafety.png)

## What it models

Each shutter is up or down; `Remaining` holds the number of shutters that can still go down (3). Each `Down` takes a token of `Remaining`, each `Up` gives it back.

## What it teaches

- [Counter pattern](../../../doc/en/03-Petri-Nets.md#6-patterns-of-modeling)
- [A P-invariant as a safety rule](../../../doc/en/04-Analysis.md#62-what-p-invariants-tell)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/SmarthomeSafety/SmarthomeSafety.json
```

1. Press **Space**, put three shutters down: the fourth `Down` is grey.
2. *Analysis > Properties*: the invariant `P1 + P3 + P5 + P7 (the shutters down) + P8 (Remaining) = 3`.
3. Put 4 tokens in `Remaining`: the rule is gone, all shutters can go down.

## Credits

Credit: YAWL User Group, [smart home safety](https://www.youtube.com/shorts/BB5N9Nxtq00).

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
