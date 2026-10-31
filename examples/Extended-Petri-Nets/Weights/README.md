# ⚗️ Weights

Arc weights: a chemical reaction, 2 H2 + O2 → 2 H2O.

![Weights](Weights.png)

## What it models

`Burn` takes 2 tokens of `H2` and 1 of `O2` and gives 2 tokens of `H2O`; `Electrolysis` does the opposite.

## What it teaches

- [Weights](../../../doc/en/05-Extended-Petri-Nets.md#1-weights)
- [Weighted P-invariants: the atoms are conserved](../../../doc/en/04-Analysis.md#62-what-p-invariants-tell)

## Try it in the editor

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Extended-Petri-Nets/Weights/Weights.json
```

1. Press **Space**, fire `Burn` twice: `H2` and `O2` are empty.
2. Fire `Electrolysis`: the reaction goes back.
3. *Analysis > Properties*: `H2 + H2O = 4` and `2 O2 + H2O = 4`.
4. Right click on the arc `H2 → Burn` > *Weight...*: change the weight and look at the invariants.

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/en/05-Extended-Petri-Nets.md)
