# Petri Net Examples

## Coffee machine

The client inserts a coin and press to the brew button to get a coffee.
The machine accept to return the inserted client coin.

![Coffee](pics/Coffee.png)

```
TimedPetriNetEditor data/examples/Coffee.json
```

## Traffic Lights

Two traffic lights (red, orange, red lights) synchronized by the Place `P6`.

![Traffic Lights](pics/TrafficLights.png)

```
TimedPetriNetEditor data/examples/TrafficLights.json
```

## German Traffic Lights

One traffic ligth. The particularity of German traffic lights is they are at the same time red and yellow before swaitching to green.

```
TimedPetriNetEditor data/examples/GermanTrafficLights.json
```

Credit: YAWL User Group
https://www.youtube.com/shorts/ZQZCpDRqwKY

## Philosphers

Three philosophers are eating but they are sharing their fork and knives.

![Philosophers](pics/Philosophers.png)

```
TimedPetriNetEditor data/examples/Philosophers.json
```

## Urgency Call (French 911)

Victims is calling the 911. A first operator (level 1) is selecting the type of urgency (advice urgent, critical):
- For an advice the operator is giving instructions to the victim.
- For a urgent case, the phone call is transfered to the operator of level 2.
- For a critical case, the phone call is transfered to the operator of level 2 but the operator of level 1 is still present with the victim.

![Appels Durgence](pics/AppelsDurgence.png)

```
TimedPetriNetEditor data/examples/AppelsDurgence.json
```

## Four Roads Junction

Simulate a road junction.

![Four Road Junctions](pics/FourRoadJunctions.png)

```
TimedPetriNetEditor data/examples/FourRoadJunctions.json
```

## Producer Consumer

Simulate a producer and a consumer with a buffer.

![Producer Consumer](pics/ProducerConsumer.png)

```
TimedPetriNetEditor data/examples/ProducerConsumer.json
```

## Inputs Outputs

Show source and sinks transitions

![Inputs Outputs](pics/InputsOutputs.png)

```
TimedPetriNetEditor data/examples/InputsOutputs.json
```

## Event Graph

Show a timed event graph taken from
https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html.

![Event Graph](pics/EventGraph.png)

```
TimedPetriNetEditor data/examples/EventGraph.json
```

## Chainsaw Safety

Show security such as dead man works on a chainsaw.

```
TimedPetriNetEditor data/examples/Chainsaw.json
```

Credit: YAWL User Group
https://www.youtube.com/shorts/V42WiF2Ib3w

## Landing Gear

Show how to prevent damaging aircraft landing gear.
- The aircraft has initially its gear down and at low speed.
- On high speed, extending gears is not possible.
- If gears are down and aircraft go to high speed a warning is triggered.


```
TimedPetriNetEditor data/examples/LandingGear.json
```

Credit: YAWL User Group
https://www.youtube.com/shorts/IqMyF5JKoE4

## Smarthome Safety

Prevent shuttering down the last roller shutters.

```
TimedPetriNetEditor data/examples/SmarthomeSafety.json
```

Credit: YAWL User Group
https://www.youtube.com/shorts/BB5N9Nxtq00
