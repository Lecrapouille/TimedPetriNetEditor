# 🤖 PickAndPlace

A robot picks an object and places it; when the gripper slips, it tries again. The result of a function chooses what comes next.

![PickAndPlace](PickAndPlace.png)

## What it models

```text
 (Ready) ─► [detect] ─► (Seen) ─► [grasp] ─► (Held) ──Success──► [place] ─► (Done)
                           ▲                    │
                           └──── [retry] ◄──────┘ Failed
```

`grasp` gives its token to `Held` **colored by its result**: the arc `Held → place` asks for `Success` tokens, the arc `Held → retry` for `Failed` ones. `retry` has no function: it fires at once and sends the object back to `Seen`.

## What it teaches

- [The idea of Symmetri: a transition is a function](../../../doc/11-Runtime.md#11-the-idea-of-symmetri)
- [The colors](../../../doc/11-Runtime.md#23-the-colors), [executors](../../../doc/11-Runtime.md#25-executors-where-the-functions-run), [the journal](../../../doc/11-Runtime.md#5-the-journal)
- [The editor as a debugger](../../../doc/13-Debugging.md)

## Run it

[`PickAndPlace.cpp`](PickAndPlace.cpp) gives a function to `detect`, `grasp` and `place`, on a pool of 4 threads; `grasp` slips twice. The goal is a token in `Done`. Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/PickAndPlace
no function (fires at once): retry
camera: object seen
gripper: slipped
gripper: slipped
gripper: holding
arm: placed
order_42: Success

{"instance":"order_42","transition":"detect","event":"Scheduled","date":0.0}
{"instance":"order_42","transition":"detect","event":"Started","date":0.0}
{"instance":"order_42","transition":"detect","event":"Completed","result":"Success","date":0.050145255}
{"instance":"order_42","transition":"grasp","event":"Scheduled","date":0.050145255}
{"instance":"order_42","transition":"grasp","event":"Started","date":0.050145255}
{"instance":"order_42","transition":"grasp","event":"Completed","result":"Failed","date":0.150282725}
{"instance":"order_42","transition":"retry","event":"Scheduled","date":0.150282725}
{"instance":"order_42","transition":"retry","event":"Completed","result":"Success","date":0.150282725}
...
{"instance":"order_42","transition":"place","event":"Completed","result":"Success","date":0.430759937}
```

`unregisteredTransitions()` names the transitions without function at startup (a forgotten function is a silent bug). The journal is the one of the instance, in JSON lines: three events per function (`Scheduled`, `Started`, `Completed` with its result).

## Watch it in the editor

[`Debugger.cpp`](Debugger.cpp) runs the same pick and place over and over, with slow functions and a gripper failing one time out of three, and publishes its journal on ZeroMQ (built when libzmq is found):

```sh
./build/examples/Debugger &            # publishes for 30 s (or: Debugger 120)
./build/TimedPetriNetEditor -c tcp://localhost:5556 examples/Runtime/PickAndPlace/PickAndPlace.json
```

The editor moves the tokens as the program runs: `place` is marked `running` while its function runs, `retry` `no function`, and the *Journal* lists the events of each order ([chapter 13](../../../doc/13-Debugging.md)).

---

Back to the [runtime examples](../README.md) · [all the examples](../../README.md) · The runtime: [chapter 11](../../../doc/11-Runtime.md)
