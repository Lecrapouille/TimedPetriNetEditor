# 🎨 Colored philosophers

The dining philosophers with colored tokens: one place for all the thinking
philosophers, one for all the eating ones.

![Colored philosophers](ColoredPhilosophers.png)

## What it models

Three philosophers sit around a table with three forks. `Phi i` eats with
`Fork i` and `Fork i+1` (`Phi2` with `Fork2` and `Fork0`).

In the uncolored [Philosophers](../../Petri-Nets/Philosophers/README.md),
each philosopher has two places of their own, *Not Eating* and *Eating*. Here
these six places are **folded** into two:

- `Thinking` starts with three tokens, colored `Phi0`, `Phi1` and `Phi2`
  (JSON: `"tokens": 3, "colors": { "Phi0": 1, "Phi1": 1, "Phi2": 1 }`);
- `Eating` starts empty.

The arc `Thinking → Phi0 eats` asks for the color `Phi0`: `Phi0 eats` takes
the token of `Phi0` only. The arc `Phi0 eats → Eating` gives a `Phi0` token,
and `Phi0 thinks` gives it back to `Thinking` with the two forks.

## What it teaches

- Colored tokens and the colors of the initial tokens:
  [chapter 5, colored tokens](../../../doc/05-Extended-Petri-Nets.md#6-colored-tokens).
- The **limit** of the colors of this editor: a color is a name, not a
  variable. The places are folded, but not the transitions: there is still one
  `eats` and one `thinks` transition per philosopher. A general colored Petri
  net would have one `eats` transition whose arcs bind a variable `p` to the
  philosopher.
- The simulation of the editor and the runtime follow the colors with the
  same rule.

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Extended-Petri-Nets/ColoredPhilosophers/ColoredPhilosophers.json
```

1. Press **Space** to start the simulation: the three `eats` transitions are
   enabled, each one by the token of its own color.
2. Fire `Phi0 eats`: `Phi1 eats` and `Phi2 eats` are no longer enabled, as
   `Fork0` and `Fork1` are taken. `Eating` holds one token, colored `Phi0`,
   so `Phi0 thinks` is the only transition that can fire. Each token is drawn
   with the color of its philosopher, the one written on its arcs.
3. Fire `Phi0 thinks`: the token `Phi0` and the two forks come back.

## Run it with the runtime

[`ColoredPhilosophers.cpp`](ColoredPhilosophers.cpp) runs the net twice:
with the engine of the editor (the simulation) and with the
[runtime](../../../doc/11-Runtime.md), both with the policy `FirstDeclared`.
It prints the colored marking at each firing of the simulation, then checks
that both fire the same transitions in the same order. Build the examples
(`make -C examples`), then from the root of the project:

```text
$ build/examples/ColoredPhilosophers
initial    Thinking: Phi0 Phi1 Phi2 | Fork0: Success | Fork1: Success | Fork2: Success
simulation Phi0 eats    Thinking: Phi1 Phi2 | Fork2: Success | going: Eating<-Phi0
simulation Phi0 thinks  Thinking: Phi1 Phi2 | Fork2: Success | going: Thinking<-Phi0 Fork0<-Success Fork1<-Success
simulation Phi0 eats    Thinking: Phi1 Phi2 | Fork2: Success | going: Eating<-Phi0
...
runtime    Thinking: Phi0 Phi1 Phi2 | Fork0: Success | Fork1: Success | Fork2: Success
12 firings, simulation and runtime agree
```

`going` lists the tokens on their way: they reach their place just after the
firing. The forks have no color of their own, so they are `Success`.

Look at who eats: always `Phi0`. With `FirstDeclared`, the first declared
transition that can fire fires first, and `Phi0 eats` can fire again as soon
as `Phi0` gives the forks back. `Phi1` and `Phi2` **starve**. The net does
not forbid them to eat; the policy never chooses them. The `Random` policy of
the [uncolored example](../../Petri-Nets/Philosophers/README.md) shares the
meals.

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/05-Extended-Petri-Nets.md)
