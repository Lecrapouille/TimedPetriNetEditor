# Controlling the editor through MQTT commands

When opening a petri json file (i.e. `foobar.json`). The editor will listening
the MQTT topic `"pneditor/foobar"` on localhost with port 1883 for MQTT
commands such as `"T0"` for triggering the transition T0 or `"P0"` for incrementing
of one the number of tokens in the place P0. Dummy file have their topic named
`"editor/petri.json"`.

TODO: implement commands such as `"T0;T1;T2"` or `"P4+2"` or `"P4-1"` or `"P4=2"`.
TBD: topics such as `"pneditor/foobar/T0"` with value `true/false` or
`"pneditor/foobar/P0"` with values such as `1`?

For example, you can type on your console:
```
sudo mosquitto_pub -h localhost -t "pneditor/foobar" -m "T0"
```

For more information concerning MQTT, read this
[document](https://www.howtoforge.com/how-to-install-mosquitto-mqtt-message-broker-on-debian-11/).
