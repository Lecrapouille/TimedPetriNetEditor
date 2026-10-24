# 📦 Producer and consumer

A producer and a consumer that share a buffer without limit.

![Producer and consumer](ProducerConsumer.png)

## What it models

The producer alternates `Produce` and `Send` (two tokens cycle between `Idle` and `Producer ready`: two producers); `Send` puts a part in `Buffer`; the consumer, `Ready` twice, takes a part with `Accept` and consumes it.

## What it teaches

- [A worked example of the firing rule, with Pre and Post](../../../doc/03-Petri-Nets.md#31-a-worked-example-the-producer-and-the-consumer)
- [Buffer pattern](../../../doc/03-Petri-Nets.md#6-patterns-of-modeling)
- [Unbounded place, coverability graph](../../../doc/04-Analysis.md#4-the-coverability-graph-karp-miller)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
```

1. Press **Space**: `Produce`, `Send` and `Accept` are green.
2. Fire `Send` many times (fire `Produce` between them): `Buffer` grows without limit.
3. *Analysis > Properties*: the exploration stops, Karp-Miller says that `Buffer` is unbounded.
4. Right click on `Buffer` > *Max tokens*, give 5: the net becomes bounded.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/03-Petri-Nets.md)
