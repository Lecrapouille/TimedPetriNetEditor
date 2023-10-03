# Petri Net Examples

## Coffee machine

The client inserts a coin and press to the brew button to get a coffee.
The machine accept to return the inserted client coin.

![Coffee](pics/Coffee.png)

```
TimedPetriNetEditor examples/Coffee.json
```

## Traffic Lights

Two traffic lights (red, orange, red lights) synchronized by the Place `P6`.

![Traffic Lights](pics/TrafficLights.png)

```
TimedPetriNetEditor examples/TrafficLights.json
```

## Philosphers

Three philosophers are eating but they are sharing their fork and knives.

![Philosophers](pics/Philosophers.png)

```
TimedPetriNetEditor examples/Philosophers.json
```

## Urgency Call (French 911)

Victims is calling the 911. A first operator (level 1) is selecting the type of urgency (advice urgent, critical):
- For an advice the operator is giving instructions to the victim.
- For a urgent case, the phone call is transfered to the operator of level 2.
- For a critical case, the phone call is transfered to the operator of level 2 but the operator of level 1 is still present with the victim.

![Appels Durgence](pics/AppelsDurgence.png)

```
TimedPetriNetEditor examples/AppelsDurgence.json
```

## Four Roads Junction

Simulate a road junction.

![Four Road Junctions](pics/FourRoadJunctions.png)

```
TimedPetriNetEditor examples/FourRoadJunctions.json
```

## Producer Consumer

Simulate a producer and a consumer with a buffer.

![Producer Consumer](pics/ProducerConsumer.png)

```
TimedPetriNetEditor examples/ProducerConsumer.json
```

## Inputs Outputs

Show source and sinks transitions

![Inputs Outputs](pics/InputsOutputs.png)

```
TimedPetriNetEditor examples/InputsOutputs.json
```

## Event Graph

Show a timed event graph taken from
https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html.

![Inputs Outputs](pics/InputsOutputs.png)

```
TimedPetriNetEditor examples/EventGraph.json
```
