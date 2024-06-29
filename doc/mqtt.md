# Controlling the editor through MQTT commands

The editor starts a MQTT client, with by default localhost with port 1883 (you can change
them from the Makefile. Search for `MQTT_BROKER_ADDR` and `MQTT_BROKER_PORT`).

For more information concerning MQTT, read this
[document](https://www.howtoforge.com/how-to-install-mosquitto-mqtt-message-broker-on-debian-11/).

## Load a new Petri net

Note: if you want to control the net through MQTT commands, better to avoid creating "timed" net
(timed event graph and timed Petri net), create instead Petri net or GRAFCET.

The message content is the same JSON format than the one used for saving/loading files. See [here](save.md)
for more information.

Example:
```
 mosquitto_pub -h localhost -t "tpne/load" -m '{ "revision": 3, "type":
"Petri net", "nets": [ { "name": "Petri net",
"places": [ { "id": 0, "caption": "P0", "tokens": 1, "x": 244, "y": 153 },
{ "id": 1, "caption": "P1", "tokens": 0, "x": 356, "y": 260 } ],
"transitions": [ { "id": 0, "caption": "T0", "x": 298, "y": 207, "angle": 0 } ],
"arcs": [ { "from": "P0", "to": "T0" }, { "from": "T0", "to": "P1", "duration": 3 }
] } ] }'
```

Constrain: the simulation shall not running when loading the net.

## Start/Stop simulation

Before firing transitions, you have to start the simulation.

```
mosquitto_pub -h localhost -t "tpne/start" -m ''
```

For stopping:
```
mosquitto_pub -h localhost -t "tpne/stop" -m ''
```

## Firing transitions

The message to send is a list of '1' or '0' characters (string). One character by transitions.

Let suppose, you have 2 transitions in your net. The message length shall be 2. Let suppose you want
to fire transition 1 but not the second. The message will be "10".

```
mosquitto_pub -h localhost -t "tpne/fire" -m '10'
```

Constrain:
- The simulation shall running.
- The message length shall match the number of transitions.