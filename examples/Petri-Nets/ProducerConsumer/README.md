# 📦 Producer and consumer

A producer and a consumer that share a buffer without limit.

![Producer and consumer](ProducerConsumer.png)

## What it models

The producer alternates `Produce` and `Send` (two tokens cycle between `Idle` and `Producer ready`: two producers); `Send` puts a part in `Buffer`; the consumer, `Ready` twice, takes a part with `Accept` and consumes it.

## What it teaches

- [A worked example of the firing rule, with Pre and Post](../../../doc/en/03-Petri-Nets.md#31-a-worked-example-the-producer-and-the-consumer)
- [Buffer pattern](../../../doc/en/03-Petri-Nets.md#6-patterns-of-modeling)
- [Unbounded place, coverability graph](../../../doc/en/04-Analysis.md#4-the-coverability-graph-karp-miller)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
```

1. Press **Space**: `Produce`, `Send` and `Accept` are green.
2. Fire `Send` many times (fire `Produce` between them): `Buffer` grows without limit.
3. *Analysis > Properties*: the exploration stops, Karp-Miller says that `Buffer` is unbounded.
4. Right click on `Buffer` > *Max tokens*, give 5: the net becomes bounded.

## Run it with the runtime

[`ProducerConsumer.cpp`](ProducerConsumer.cpp) runs this net with the runtime of the project ([chapter 11](../../../doc/en/11-Runtime.md)), on a pool of threads, and **restarts it after a crash**. `Produce` makes a part (a guard stops at 6 parts), `Consume` uses one; the two consumers (`Ready` has two tokens) work at the same time. The journal of the instance is written line by line in `build/examples/ProducerConsumer.jsonl` (`Options::journal`). The program "crashes" while two parts are used: the journal has the `Started` of these two `Consume`, not their `Completed`. The second run reads the journal, rebuilds the instance with `restore()` (the marking is replayed from the initial tokens) and calls the two `Consume` again (`Recovery::Restart`: the function shall be idempotent). Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/ProducerConsumer
First run
  producer: part 1 made
  producer: part 2 made
  producer: part 3 made
  consumer: part 1 used
  producer: part 4 made
  consumer: part 2 used
  producer: part 5 made
  crash! (while the parts 3 to 4 are used)
Second run
  journal: 47 lines, 5 parts made, 2 used
  restored: Buffer 5, Accepted 0, Ready 0; Consume running at the crash: 2, called again
  producer: part 6 made
  consumer: part 3 used
  consumer: part 4 used
  consumer: part 5 used
  consumer: part 6 used
  consumer: part 7 used
  consumer: part 8 used
  consumer: part 9 used
  consumer: part 10 used
Done: 6 parts made, 10 used (expected 10)
```

No part is lost and none is used twice: 3 parts were in `Buffer` at the start, 1 was ready to be sent, 6 were made. The counters of the program are rebuilt from the journal too (the `Completed` of `Produce` and `Consume`). With `Recovery::Fail`, the two `Consume` would end with `Failed` instead of being called again.

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
