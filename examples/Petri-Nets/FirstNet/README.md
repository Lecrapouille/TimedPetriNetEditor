# 👋 First net

The smallest Petri net: one place, one transition, one place.

![First net](FirstNet.png)

## What it models

Nothing but the firing rule: a token in `Ready`, a transition `Fire`, an empty place `Done`.

## What it teaches

- [Place, transition, arc, token](../../../doc/en/03-Petri-Nets.md#2-the-elements-of-a-petri-net)
- [The firing rule](../../../doc/en/03-Petri-Nets.md#3-the-firing-rule)
- [A wanted deadlock: the end of all activity](../../../doc/en/04-Analysis.md#31-deadlock)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Petri-Nets/FirstNet/FirstNet.json
```

1. Press **Space**: the frame turns green, the simulation runs.
2. Click on the green transition `Fire`: the token leaves `Ready`, a token appears in `Done`.
3. `Fire` is grey: it is no longer enabled, nothing can happen any more.
4. Press **Space** again: the initial marking comes back. Put the mouse on `Ready`, press **+** twice, and run again: `Fire` can now fire three times.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
