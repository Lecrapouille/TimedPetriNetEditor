# 🌐 MicroServices

An order calls a payment service, which answers by a message; a timeout stops waiting for a service that is too slow.

## What it models

The net is built in the code ([`MicroServices.cpp`](MicroServices.cpp)):

```text
(New) -> [request] -> (Waiting) -> [confirm] -> (Paid)
                          |            ^
                          |      (Answer) <- [paid]   <- inject(): the message of the service
                          |
                          \-> [timeout] 0.5 s -> (Error)
```

`request` calls the fake HTTP service, which answers on its own thread after its delay with `inject("paid")`. `timeout` has a **delay** of 0.5 s and is in conflict with `confirm` on the token of `Waiting`: the first one able to fire wins. This is the pattern of [the timeout of chapter 6](../../Timed-Petri-Nets/Timeout/README.md), in a program.

## What it teaches

- [Sources and injected events](../../../doc/11-Runtime.md#32-sources-and-injected-events)
- [Timeouts](../../../doc/11-Runtime.md#34-timeouts)
- [Journals for spreadsheets and process mining](../../../doc/13-Debugging.md#81-spreadsheets-and-process-mining)

## Run it

Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/MicroServices
fast service: paid
slow service: timeout
saved build/examples/Order.json
saved build/examples/Order.pnml
saved build/examples/Order.dot
saved build/examples/orders.csv
saved build/examples/orders.xes
```

The fast service answers in 0.1 s: paid. The slow one answers in 1 s: the timeout fired at 0.5 s, the late answer is ignored. The net is saved by the library of the file formats (JSON for the editor, PNML, Graphviz), the journals of the two orders as CSV and XES:

```text
instance,transition,event,result,date
fast,request,Scheduled,,0
fast,request,Started,,0
fast,request,Completed,Success,0.0101037
fast,paid,Scheduled,,0.100693
...
```

Open `build/examples/Order.json` in the editor to see the net, and `orders.xes` in a process mining tool (ProM, PM4Py).

---

Back to the [runtime examples](../README.md) · [all the examples](../../README.md) · The runtime: [chapter 11](../../../doc/11-Runtime.md)
