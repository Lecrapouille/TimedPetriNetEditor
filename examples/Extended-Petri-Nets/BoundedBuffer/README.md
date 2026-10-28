# 🏭 Bounded buffer

Four extensions in one production line: capacity, weight, inhibitor arc, reset arc.

![Bounded buffer](BoundedBuffer.png)

## What it models

`Produce` puts parts in `Buffer` (capacity 4); `Pack` takes 2 parts at once (weight 2); `Produce` is blocked while `Stopped` has a token (inhibitor arc); `Flush` empties `Buffer` (reset arc). `Stop`, `Restart` and `Ask flush` let you act on the line.

## What it teaches

- [All the extensions together](../../../doc/en/05-Extended-Petri-Nets.md#7-all-together-the-bounded-buffer)
- [What they cost](../../../doc/en/05-Extended-Petri-Nets.md#8-what-the-extensions-cost)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.json
```

1. Press **Space**, fire `Produce` until `Buffer` is full (4): `Produce` turns grey.
2. Fire `Pack`: 2 parts go at once.
3. Fire `Stop`: `Produce` is blocked. `Restart` releases it.
4. Fire `Ask flush`, then `Flush`: the buffer is empty.

## Run it with the runtime

[`BoundedBuffer.cpp`](BoundedBuffer.cpp) runs this net with the runtime of the project: each transition is a function of the program ([chapter 11](../../../doc/en/11-Runtime.md)). Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/BoundedBuffer
 date  Buffer  Packed  Stopped  event
    0       2       0        0  
    1       3       0        0  
    2       4       0        0  
    3       3       1        0  
    4       4       1        0  
    5       4       1        0  
    6       3       2        0  
    7       4       2        0  
    8       4       2        1  Stop: the producer waits
    9       2       3        1  
   10       2       3        1  
   11       2       3        1  
   12       1       4        0  Restart
   13       2       4        0  
   14       3       4        0  
   15       0       5        0  Ask flush: Buffer emptied
   16       1       5        0  
   17       2       5        0  
   18       1       6        0  
   19       2       6        0  
   20       3       6        0
```

The runtime runs the line on a simulated clock: guards make the producer give a part every time unit and the packer pack 2 parts every 3. The program injects `Stop` at 8 and `Ask flush` at 15 (transitions without input place, like buttons), and opens the guard of `Restart` at 12. At 2, 4, 5 and 7 the full buffer blocks the producer (capacity); from 8 to 11 the inhibitor arc stops it; at 15 the reset arc empties the buffer.

---

Back to the [examples](../../README.md) · Extended Petri nets: [chapter 5](../../../doc/en/05-Extended-Petri-Nets.md)
