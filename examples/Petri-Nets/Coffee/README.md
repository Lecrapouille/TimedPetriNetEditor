# ☕ Coffee machine

A coffee machine that accepts or rejects coins, then serves a coffee.

![Coffee machine](Coffee.png)

## What it models

The water is `Cold` or `Hot` (`Brew` heats it). The client inserts a coin; the machine accepts it or gives it back. `Dispense` needs hot water **and** an accepted coin, then the machine is ready again and the water cold. The arcs of the choice have the colors `Success` (accept) and `Failed` (reject) for the runtime.

## What it teaches

- [Choice then synchronization](../../../doc/03-Petri-Nets.md#6-patterns-of-modeling)
- [P-invariants: the water and the coin each in one state](../../../doc/04-Analysis.md#6-p-invariants)
- [Colors of results](../../../doc/05-Extended-Petri-Nets.md#6-colored-tokens)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/Coffee/Coffee.json
```

1. Press **Space**. Fire `Insert Coin`, then `Reject Coin`: the coin is given back.
2. Insert again, `Accept`, then `Brew`, then `Dispense`: a coffee.
3. *Analysis > Properties*: `Cold + Hot = 1` and `Ready + Coin Inserted + Accepted = 1`; live, no deadlock.

## Run it with the runtime

[`Coffee.cpp`](Coffee.cpp) runs this net with the runtime of the project: each transition is a function of the program ([chapter 11](../../../doc/11-Runtime.md)). Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/Coffee
  heater: water is hot
coin 1: good
  coin accepted
  coffee 1 served
  heater: water is hot
coin 2: fake
  coin given back
coin 3: good
  coin accepted
  coffee 2 served
  heater: water is hot
coin 4: good
  coin accepted
  coffee 3 served
  heater: water is hot
3 coffees for 4 coins
```

Each transition is a function of the machine. `Insert Coin` is the coin checker: it returns `Success` for a good coin and `Failed` for a fake one, so the token in `Coin Inserted` has this color and the arcs choose `Accept` or `Reject Coin`. A guard lets `Insert Coin` fire only while clients wait. The second coin is fake: it is given back, and three coffees are served for four coins.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
