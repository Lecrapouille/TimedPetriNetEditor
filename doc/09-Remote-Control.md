# 9. Remote control

What you will find here: how another program (a test, a robot, a script)
drives a running editor: load a net, start it, fire transitions, read the
tokens; and how the editor follows a program running a net (§4).

---

## 1. How it works

The editor listens on `tcp://*:5555` with [ZeroMQ](https://zeromq.org/) (a
REP socket). A client sends one JSON command and receives one JSON answer.

```text
  your program  ── {"command": "fire", "params": {...}} ──►  editor
                ◄── {"status": "ok", "message": "..."} ───
```

The remote control starts with the editor (the console says
`Remote control listening on tcp://*:5555`); *View > Remote control*
stops or starts it. Build without it with `make TPNE_ZEROMQ=0`.

## 2. Tutorial: drive the producer-consumer

Open `data/examples/ProducerConsumer.json` in the editor, then run this
Python script (`pip install pyzmq`):

```python
import json, time, zmq
s = zmq.Context().socket(zmq.REQ)
s.connect("tcp://localhost:5555")
def send(cmd):
    print(">>", json.dumps(cmd))
    s.send_string(json.dumps(cmd))
    print("<<", s.recv_string())

send({"command": "state"})
send({"command": "start"})
time.sleep(0.5)
send({"command": "fire", "params": {"transitions": [0]}})
time.sleep(0.5)
send({"command": "state"})
send({"command": "stop"})
```

It prints (the lists of transitions are cut here):

```text
>> {"command": "state"}
<< {"places":[{"caption":"Producer ready","id":0,"key":"P0","tokens":1},{"caption":"Idle","id":1,"key":"P1","tokens":1},{"caption":"Buffer","id":2,"key":"P2","tokens":3},{"caption":"Accepted","id":3,"key":"P3","tokens":0},{"caption":"Ready","id":4,"key":"P4","tokens":2}],"running":false,"status":"ok","transitions":[...]}
>> {"command": "start"}
<< {"message":"Simulation started","status":"ok"}
>> {"command": "fire", "params": {"transitions": [0]}}
<< {"message":"Transitions fired","status":"ok"}
>> {"command": "state"}
<< {"places":[{"caption":"Producer ready","id":0,"key":"P0","tokens":0},{"caption":"Idle","id":1,"key":"P1","tokens":2},{"caption":"Buffer","id":2,"key":"P2","tokens":4},{"caption":"Accepted","id":3,"key":"P3","tokens":0},{"caption":"Ready","id":4,"key":"P4","tokens":2}],"running":true,"status":"ok","transitions":[...]}
>> {"command": "stop"}
<< {"message":"Simulation stopped","status":"ok"}
```

`Send` (`T0`) fired: `Producer ready` lost its token, `Idle` and `Buffer`
got one.

## 3. Reference: commands

A command is `{"command": "<name>", "params": {...}}`. An answer has
`"status": "ok"` or `"error"` and a `"message"`.

| Command | Params | Does |
|---------|--------|------|
| `load` | `{"net": {...}}` (the JSON of [chapter 6](06-File-Format.md)) or `{"path": "file.json"}` | replace the active net; refused while simulating |
| `start` | | start the simulation; refused for timed nets |
| `stop` | | stop the simulation |
| `fire` | `{"transitions": [0, 2]}` or `{"bitfield": "101"}` (one character per transition) | make these transitions fire (Petri nets); refused when not simulating |
| `state` | | `running`, the places (`id`, `key`, `caption`, `tokens`), the transitions (`id`, `key`, `caption`, `receptivity`) |
| `set_tokens` | `{"place": 1, "tokens": 3}` | set the tokens of a place |
| `clear` | | clear the active net; refused while simulating |

Use Petri nets or GRAFCETs: the timed nets move their tokens by themselves.

The CI runs `tests/remote/test_remote.py` (pyzmq): it drives the editor with
these commands and checks the answers.

## 4. Follow a running program

The other way round: a program running a net tells the editor what it does,
and the editor shows it (a visual debugger). The program publishes on a
ZeroMQ PUB socket (`tcp://*:5556` by default); the editor subscribes with
*Debug > Connect* (or `TimedPetriNetEditor -c tcp://localhost:5556 net.json`).
The *Journal* follows the program (its *Replay* shows the marking at the last
date), the *Program* window shows its inputs and outputs and the
transitions without function, marked on the net.

Who publishes:

- a program using the runtime: `Runtime/JournalPublisher.hpp`
  (`examples/runtime/Debugger.cpp`, and the programs of *File > Export > C++
  functions for the runtime* built with `-DTPNE_DEBUG`);
- the GRAFCET code of *File > Export > C++ GRAFCET* built with
  `-DGRAFCET_DEBUG` (on an Arduino, through `tools/serial-bridge.py`).

```c++
#include "Runtime/JournalPublisher.hpp"   // link with -lzmq

JournalPublisher publisher(net);           // tcp://*:5556
Runtime::Options options;
options.journal = [&publisher](LogEntry const& e) { publisher.publish(e); };
Runtime runtime(net, executor, options);
runtime.on("grasp", grasp);
publisher.manifest(runtime);               // its transitions without function
auto order = runtime.instantiate("order_42");
publisher.marking(*order);                 // for an editor connected later
```

Each message is one JSON object:

| Message | Example | The editor |
|---------|---------|------------|
| a line of the journal | `{"instance":"order_42","transition":"grasp","event":"Completed","result":"Failed","date":1.5}` | adds it to the *Journal* |
| manifest | `{"type":"manifest","net":"Pick and place","places":[...],"transitions":[...],"unregistered":["retry"]}` | marks the transitions without function (sent again every second) |
| marking | `{"type":"marking","instance":"order_42","date":1.5,"tokens":[1,0,0,0]}` | starts the replay of the instance from these tokens |
| inputs and outputs | `{"type":"io","inputs":{"Start":1},"outputs":{"Motor":true}}` | shows them in the *Program* window (GRAFCET) |

`event` is `Scheduled` (the transition took its tokens), `Started` (its
function runs) or `Completed` (it gave its tokens, colored by `result`). A
GRAFCET publishes an instance per graph (its name) and the keys of the
transitions (`T3`). *Debug > Open a manifest* marks the transitions without
function from a file written by `toManifest(runtime)`.

---

Next: [10. Use the library in C++](10-Library.md).
