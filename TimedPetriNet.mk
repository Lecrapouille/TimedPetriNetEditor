# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# The libraries of TimedPetriNetEditor, without its editor, for MyMakefile 3.
# Their headers are included from src/ ("PetriNet/Net.hpp") and each one
# installs the headers of its folder. A project embedding them reads this
# file with their third parties:
#   TARGETS += TimedPetriNetEditor
#   TimedPetriNetEditor.GIT := https://github.com/Lecrapouille/TimedPetriNetEditor.git
#   TimedPetriNetEditor.MAKEFILES := TimedPetriNet.mk external/external.mk
#   myapp.DEPENDS := TimedPetriRuntime TimedPetriIO
# TimedPetriViewer also needs the target imgui: declared by the project, or
# by reading external/gui.mk too.

TARGETS += tpne-src TimedPetriNet TimedPetriIO TimedPetriMaxPlus \
    TimedPetriRuntime TimedPetriAnalysis TimedPetriViewer

# The include folder of all the libraries.
tpne-src.PUBLIC_INCLUDES := src
tpne-src.INSTALL := no

TimedPetriNet.TYPE := library
TimedPetriNet.DESCRIPTION := Library manipulating Petri nets
TimedPetriNet.SOURCES := src/PetriNet/**/*.cpp
TimedPetriNet.PUBLIC_HEADERS := src/PetriNet/**/*.hpp
TimedPetriNet.INCLUDES := src
TimedPetriNet.DEPENDS := tpne-src json
TimedPetriNet.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-nan-infinity-disabled \
    -Wno-format-nonliteral -Wno-double-promotion -Wno-cast-qual -Wno-float-equal -fno-fast-math

TimedPetriIO.TYPE := library
TimedPetriIO.DESCRIPTION := Load and save Petri nets in many file formats
TimedPetriIO.SOURCES := src/IO/**/*.cpp
TimedPetriIO.PUBLIC_HEADERS := src/IO/**/*.hpp
TimedPetriIO.INCLUDES := src
TimedPetriIO.DEPENDS := TimedPetriNet tinyxml2
TimedPetriIO.MERGE := tinyxml2
TimedPetriIO.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal \
    -Wno-nan-infinity-disabled -Wno-format-nonliteral -Wno-double-promotion -fno-fast-math

TimedPetriMaxPlus.TYPE := library
TimedPetriMaxPlus.DESCRIPTION := (max,+) algebra and analyses of timed event graphs
TimedPetriMaxPlus.SOURCES := src/MaxPlus/**/*.cpp src/MaxPlus/**/*.c
TimedPetriMaxPlus.PUBLIC_HEADERS := src/MaxPlus/**/*.hpp src/MaxPlus/**/*.h
TimedPetriMaxPlus.INCLUDES := src
TimedPetriMaxPlus.DEPENDS := TimedPetriIO
TimedPetriMaxPlus.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-cast-qual \
    -Wno-float-equal -Wno-nan-infinity-disabled -fno-fast-math
TimedPetriMaxPlus.CFLAGS := -Wno-sign-conversion -Wno-float-equal -Wno-nan-infinity-disabled -fno-fast-math

TimedPetriRuntime.TYPE := library
TimedPetriRuntime.DESCRIPTION := Runtime calling functions when the transitions of a net fire
TimedPetriRuntime.SOURCES := src/Runtime/**/*.cpp
TimedPetriRuntime.PUBLIC_HEADERS := src/Runtime/**/*.hpp
TimedPetriRuntime.INCLUDES := src
TimedPetriRuntime.DEPENDS := TimedPetriNet
TimedPetriRuntime.CXXFLAGS := -Wno-old-style-cast -Wno-float-equal -Wno-noexcept -fno-fast-math

TimedPetriAnalysis.TYPE := library
TimedPetriAnalysis.DESCRIPTION := Properties of Petri nets: reachability, deadlocks, bounds, liveness, invariants
TimedPetriAnalysis.SOURCES := src/Analysis/**/*.cpp
TimedPetriAnalysis.PUBLIC_HEADERS := src/Analysis/**/*.hpp
TimedPetriAnalysis.INCLUDES := src
TimedPetriAnalysis.DEPENDS := TimedPetriNet
TimedPetriAnalysis.CXXFLAGS := -Wno-old-style-cast

# Draws a net, read only, in the Dear ImGui window of a program. The project
# declares imgui (external/gui.mk here).
TimedPetriViewer.TYPE := static
TimedPetriViewer.DESCRIPTION := Draw a Petri net in a Dear ImGui window
TimedPetriViewer.SOURCES := src/Editor/NetViewer.cpp src/Editor/Drawable.cpp src/Editor/Theme.cpp
TimedPetriViewer.PUBLIC_HEADERS := src/Editor/NetViewer.hpp src/Editor/Drawable.hpp src/Editor/Theme.hpp
TimedPetriViewer.INCLUDES := src
TimedPetriViewer.DEPENDS := TimedPetriNet imgui
TimedPetriViewer.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal \
    -Wno-double-promotion -Wno-conversion -Wno-useless-cast
