# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

##=====================================================================
## TimedPetriNetEditor: the examples run by the runtime (the *.cpp files
## next to their net, in the folders of examples/).
## Build the project first (make at its root), then: make -C examples
##=====================================================================

TPNE := $(abspath ..)
BUILD := $(TPNE)/build/examples
CXXFLAGS := -std=c++20 -Wall -Wextra -I$(TPNE)/src -I$(TPNE)/external -I$(TPNE)/external/json/include
LIBS := -L$(TPNE)/build -lTimedPetriRuntime -lTimedPetriIO -lTimedPetriNet -Wl,-rpath,$(TPNE)/build -pthread

# Every program of the examples, but the ones needing what may be missing:
# Ros2Node.cpp needs rclcpp (not built here), Debugger.cpp needs libzmq (built
# when it is found) and SymmetriBenchmark.cpp needs Symmetri.
SOURCES := $(filter-out %/Ros2Node.cpp %/Debugger.cpp %/SymmetriBenchmark.cpp,$(shell find . -name '*.cpp'))
ifneq ($(shell pkg-config --exists libzmq 2>/dev/null && echo 1),)
SOURCES += ./Runtime/PickAndPlace/Debugger.cpp
endif
EXAMPLES := $(addprefix $(BUILD)/,$(basename $(notdir $(SOURCES))))
vpath %.cpp $(sort $(dir $(SOURCES)))

all: $(EXAMPLES) no-exceptions

# The examples follow the headers and the libraries of the project.
HEADERS := $(wildcard $(TPNE)/src/*/*.hpp)
DEPENDS := $(HEADERS) $(wildcard $(TPNE)/build/libTimedPetri*.so)

$(BUILD)/Debugger: Debugger.cpp $(DEPENDS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $< $(LIBS) $(shell pkg-config --cflags --libs libzmq) -o $@

$(BUILD)/%: %.cpp $(DEPENDS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $< $(LIBS) -o $@

$(BUILD):
	mkdir -p $@

# The core and the runtime compile without exceptions nor RTTI, as on a
# micro-controller.
no-exceptions: | $(BUILD)
	$(CXX) -std=c++20 -fno-exceptions -fno-rtti -I$(TPNE)/src -I$(TPNE)/external -c \
	    $(TPNE)/src/Runtime/Runtime.cpp -o $(BUILD)/Runtime-noexcept.o
	$(CXX) -std=c++20 -fno-exceptions -fno-rtti -I$(TPNE)/src -I$(TPNE)/external -c \
	    $(TPNE)/src/PetriNet/Engine.cpp -o $(BUILD)/Engine-noexcept.o
	$(CXX) -std=c++20 -fno-exceptions -fno-rtti -I$(TPNE)/src -I$(TPNE)/external -c \
	    $(TPNE)/src/PetriNet/Firing.cpp -o $(BUILD)/Firing-noexcept.o

# Run the programs that end by themselves, from the root of the project (they
# load their net by its path from there).
RUN := PickAndPlace MicroServices TrafficLight Coffee LandingGear Colors BoundedBuffer EmergencyCalls \
       Priorities
run: all
	cd $(TPNE) && $(foreach e,$(RUN),$(BUILD)/$(e) &&) true

.PHONY: all no-exceptions run
