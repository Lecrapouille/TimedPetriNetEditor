# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# The library loaded by TimedPetriNetEditor.jl. Paths are relative to this
# directory.

TARGETS += TimedPetriJulia
TimedPetriJulia.TYPE := library
TimedPetriJulia.DESCRIPTION := Timed Petri Net Editor interface for Julia
TimedPetriJulia.SOURCES := Julia.cpp
TimedPetriJulia.PUBLIC_HEADERS := Julia.hpp
TimedPetriJulia.INCLUDES := ..
TimedPetriJulia.DEPENDS := TimedPetriGUI
TimedPetriJulia.CXXFLAGS := -Wno-cast-qual -Wno-undef
