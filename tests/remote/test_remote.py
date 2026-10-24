#!/usr/bin/env python3
# Integration tests of the ZeroMQ links of TimedPetriNetEditor (pyzmq):
#   - the remote control of the editor (REQ/REP, doc/09-Remote-Control.md):
#     load, start, fire, state, set_tokens, stop on ProducerConsumer.json;
#   - the journal published by a program (PUB/SUB, Runtime/JournalPublisher.hpp):
#     the manifest and the lines of the journal of examples/Runtime/PickAndPlace/Debugger.
# Run from the root of the project, after make and make -C examples:
#   python3 tests/remote/test_remote.py [build/TimedPetriNetEditor] [build/examples/Debugger]
# On a machine without display, run it under xvfb-run.

import json
import os
import subprocess
import sys
import time

import zmq

EDITOR = sys.argv[1] if len(sys.argv) > 1 else "build/TimedPetriNetEditor"
DEBUGGER = sys.argv[2] if len(sys.argv) > 2 else "build/examples/Debugger"
FAILURES = []


def check(condition, what):
    print(("  ok    " if condition else "  FAIL  ") + what)
    if not condition:
        FAILURES.append(what)


def request(socket, command, params=None):
    message = {"command": command}
    if params is not None:
        message["params"] = params
    socket.send_string(json.dumps(message))
    if socket.poll(5000) == 0:
        raise TimeoutError("no answer to " + command)
    return json.loads(socket.recv_string())


def test_remote_control():
    print("Remote control of the editor (REQ/REP tcp://localhost:5555)")
    env = dict(os.environ, XDG_CONFIG_HOME="/tmp/tpne-remote-test")
    editor = subprocess.Popen([EDITOR, "examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json"], env=env,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    context = zmq.Context()
    try:
        socket = context.socket(zmq.REQ)
        socket.setsockopt(zmq.LINGER, 0)
        socket.connect("tcp://localhost:5555")
        state = None
        for _ in range(50):  # the editor starts
            try:
                state = request(socket, "state")
                break
            except TimeoutError:
                socket.close()
                socket = context.socket(zmq.REQ)
                socket.setsockopt(zmq.LINGER, 0)
                socket.connect("tcp://localhost:5555")
        check(state is not None and state["status"] == "ok", "state answered")
        places = state["places"]
        check(len(places) == 5 and places[0]["tokens"] == 1, "net loaded: 5 places, P0 has a token")
        check(not state["running"], "not running")

        check(request(socket, "fire", {"transitions": [0]})["status"] == "error",
              "fire refused while not simulating")
        check(request(socket, "start")["status"] == "ok", "start")
        time.sleep(0.5)
        check(request(socket, "fire", {"transitions": [0]})["status"] == "ok", "fire T0")
        time.sleep(1.0)
        state = request(socket, "state")
        check(state["running"], "running")
        check(state["places"][0]["tokens"] == 0 and state["places"][2]["tokens"] == 4,
              "T0 took the token of P0 and gave one to the buffer P2")
        check(request(socket, "set_tokens", {"place": 0, "tokens": 3})["status"] == "ok", "set_tokens")
        check(request(socket, "state")["places"][0]["tokens"] == 3, "P0 has 3 tokens")
        check(request(socket, "unknown")["status"] == "error", "unknown command refused")
        check(request(socket, "stop")["status"] == "ok", "stop")
        socket.close()
    finally:
        editor.terminate()
        try:
            editor.wait(10)
        except subprocess.TimeoutExpired:
            editor.kill()
        context.term()


def test_journal_publisher():
    print("Journal published by a program (PUB/SUB tcp://localhost:5556)")
    program = subprocess.Popen([DEBUGGER, "6"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    context = zmq.Context()
    try:
        socket = context.socket(zmq.SUB)
        socket.setsockopt(zmq.LINGER, 0)
        socket.setsockopt_string(zmq.SUBSCRIBE, "")
        socket.connect("tcp://localhost:5556")
        manifests, entries, markings = [], [], []
        end = time.time() + 5.0
        while time.time() < end:
            if socket.poll(200) == 0:
                continue
            message = json.loads(socket.recv_string())
            kind = message.get("type")
            if kind == "manifest":
                manifests.append(message)
            elif kind == "marking":
                markings.append(message)
            else:
                entries.append(message)
        check(len(manifests) > 0, "manifest received")
        if manifests:
            check(manifests[0]["net"] == "Pick and place", "manifest of the net Pick and place")
            check(manifests[0]["unregistered"] == ["retry"], "retry has no function")
        check(len(entries) > 0, "lines of the journal received")
        events = {e.get("event") for e in entries}
        check({"Scheduled", "Started", "Completed"} <= events, "Scheduled, Started and Completed events")
        check(all("instance" in e and "transition" in e and "date" in e for e in entries),
              "lines have an instance, a transition and a date")
        check(len(markings) > 0 and len(markings[0]["tokens"]) == 4, "markings of 4 places")
        socket.close()
    finally:
        program.terminate()
        program.wait(10)
        context.term()


if __name__ == "__main__":
    test_remote_control()
    test_journal_publisher()
    print("FAILED: " + ", ".join(FAILURES) if FAILURES else "All passed")
    sys.exit(1 if FAILURES else 0)
