#!/bin/bash

# g++ -W -Wall --std=c++11 -g -O2 -DBACKWARD_HAS_DW=1 backward.cpp GUI.cpp Petri.cpp main.cpp -o TimedPetriNetEditor `pkg-config --cflags --libs sfml-graphics` -ldw

g++ -W -Wall --std=c++11 GUI.cpp Petri.cpp main.cpp -o TimedPetriNetEditor `pkg-config --cflags --libs sfml-graphics`
