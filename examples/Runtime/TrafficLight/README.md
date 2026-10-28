# 🚦 TrafficLight

A traffic light written as an Arduino sketch: `setup()`, `loop()`, one thread, no exception.

## What it models

The net is built in `setup()` (a board has no file system), in [`TrafficLight.cpp`](TrafficLight.cpp):

```text
(red) -> [go] 3 s -> (green) -> [slow] 3 s -> (orange) -> [stop] 1 s -> (red)
                        \                        ^
 [button] -> (request) -> [hurry] (priority 1) --/
```

The **delays** of the transitions make the light wait: 3 s on red, 3 s on green, 1 s on orange. The pedestrian button is a source, injected by the program; `hurry` cuts the green short, with a higher priority than `slow`. The functions are synchronous and switch the lamps.

## What it teaches

- [Targets: the micro-controller](../../../doc/en/11-Runtime.md#6-targets): `InlineExecutor`, `Options::log = false`, `step()` in `loop()` with the time of `millis()`
- [Time: durations and delays](../../../doc/en/11-Runtime.md#33-time-durations-and-delays), [conflicts and priorities](../../../doc/en/11-Runtime.md#35-conflicts-and-priorities)

## Run it

On a computer, `main()` plays the board: 20 s of `loop()` every 10 ms, a pedestrian at 4.5 s. Build the examples (`make -C examples`), then:

```text
$ build/examples/TrafficLight
     0 ms: red
  3000 ms: green
  4500 ms: pedestrian button
  4500 ms: orange (pedestrian)
  5500 ms: red
  8500 ms: green
 11500 ms: orange
 12500 ms: red
 15500 ms: green
 18500 ms: orange
 19500 ms: red
```

The button at 4.5 s cuts the first green (1.5 s instead of 3). `make -C examples` also compiles the core and the runtime with `-fno-exceptions -fno-rtti`, as on a board.

## On a board

For a board with a C++17 standard library (ESP32, RP2040, STM32 with arduino-pico or STM32duino): compile `src/PetriNet/PetriNet.cpp`, `Firing.cpp`, `Engine.cpp` and `src/Runtime/Runtime.cpp` with `-fno-exceptions -fno-rtti`, keep `setup()` and `loop()`, and remove `main()`. After `setup()`, the runtime only allocates memory for the requests of `inject()` and for the tokens on their way along arcs with a duration. A small AVR without the standard library is out of reach: for it, [generate the code of a GRAFCET](../../../doc/en/12-Code-Generation.md).

---

Back to the [runtime examples](../README.md) · [all the examples](../../README.md) · The runtime: [chapter 11](../../../doc/en/11-Runtime.md)
