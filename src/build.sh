#!/bin/bash -e

(cd .. && make clean && make -j$(nproc) && sudo make install && TimedPetriNetEditor)
