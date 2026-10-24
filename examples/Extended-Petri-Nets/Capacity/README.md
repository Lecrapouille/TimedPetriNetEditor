# 🅿️ Capacity

Place capacity: a car park of 3 spaces.

![Capacity](Capacity.png)

## What it models

`Arrive` has no input place: it is enabled until the car park (capacity 3, drawn `≤ 3`) is full. `Leave` frees a space.

## What it teaches

- [Capacities](../../../doc/05-Extended-Petri-Nets.md#2-capacities)
- [What capacities cost to the analysis](../../../doc/05-Extended-Petri-Nets.md#8-what-the-extensions-cost)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Extended-Petri-Nets/Capacity/Capacity.json
```

1. Press **Space**, fire `Arrive` three times: it turns grey.
2. Fire `Leave`: `Arrive` is enabled again.
3. Right click on `Car park` > *Max tokens*: change the capacity.
4. *Analysis > Properties*: `Gone` grows for ever and Karp-Miller is refused ("places with a capacity").

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/05-Extended-Petri-Nets.md)
