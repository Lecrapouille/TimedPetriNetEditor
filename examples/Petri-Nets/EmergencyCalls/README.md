# 🚑 Emergency calls

An emergency call center (the French 15/112), with two levels of operators.

![Emergency calls](EmergencyCalls.png)

## What it models

Seven victims call. Four level 1 operators answer and screen the calls: **advice** they give themselves; **urgent** calls they transfer to a level 2 operator (the victim waits with hold music) and hang up; **critical** calls they keep until a level 2 operator joins them in a three-way call. The four level 2 operators then give instructions. The arcs leaving `Call screening` have the colors `Advice`, `Urgent` and `Critical` for the runtime. The numbers written on the arcs are durations in minutes, kept in the file but not used: this net is a Petri net.

## What it teaches

- [A real-life net combining all the patterns](../../../doc/en/03-Petri-Nets.md#63-a-real-life-net-the-emergency-call-center)
- [Resources, choices, merges](../../../doc/en/03-Petri-Nets.md#6-patterns-of-modeling)
- [State explosion: 25 443 markings](../../../doc/en/04-Analysis.md#23-the-limit-state-explosion)
- [Colors of results](../../../doc/en/05-Extended-Petri-Nets.md#6-colored-tokens)

## Try it in the editor

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/EmergencyCalls/EmergencyCalls.json
```

1. Press **Space**, fire `Level 1 answers` four times: all the level 1 operators are busy.
2. Send calls to `Urgent` and `Critical`: watch them wait in `Hold music` and `Waiting for a three-way call` when level 2 is busy.
3. *Analysis > Properties* (*Compute*): 25 443 markings; the end, all calls handled, is a deadlock; the P-invariants say that each level always has 4 operators.
4. Change the type to *Timed Petri net* in the toolbar and press **Space**: the durations are used and the calls run by themselves ([chapter 6](../../../doc/en/06-Timed-Petri-Nets.md)).

## Run it with the runtime

[`EmergencyCalls.cpp`](EmergencyCalls.cpp) runs this net with the runtime of the project: each transition is a function of the program ([chapter 11](../../../doc/en/11-Runtime.md)). Build the examples (`make -C examples`), then from the root of the project:

```text
$ build/examples/EmergencyCalls
  1  Level 1 answers      -> Critical
  2  Critical            
  3  Level 2 answers     
  4  Level 1 leaves      
  5  End of call         
  6  Level 1 answers      -> Advice
  7  Level 1 answers      -> Urgent
  8  Urgent              
  9  Level 1 answers      -> Urgent
 10  Urgent              
 11  Level 1 answers      -> Advice
 12  Transfer to level 2 
 13  Level 1 answers      -> Critical
 14  Critical            
 15  Level 2 answers     
 16  Level 1 leaves      
 17  End of call         
 18  Level 1 answers      -> Urgent
 19  Urgent              
 20  Transfer to level 2 
 21  Transfer to level 2 
 22  Level 2 answers     
 23  Level 2 answers     
 24  Level 2 answers     
 25  Advice              
 26  Advice              
 27  End of call         
 28  End of call         
 29  End of call         
 30  End of call         
 31  End of call         
Deadlocked: 3 urgent and 2 critical calls handled by level 2
```

When a level 1 operator answers, its function returns the kind of the call as a color (`Advice`, `Urgent` or `Critical`, colors of the net): the arcs from `Call screening` take only the tokens of their color, so the kind of the call chooses its way. The list prints the transitions in the order they ended. The instance ends `Deadlocked`: nothing more can happen, all the calls are handled (no goal was given).

## Credits

A video of an older version: [YouTube](https://youtu.be/hOhunzgFpcA).

---

Back to the [examples](../../README.md) · Petri nets: [chapter 3](../../../doc/en/03-Petri-Nets.md)
