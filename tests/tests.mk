# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# Unit tests, with Google Test. Paths are relative to this directory.

TARGETS += TimedPetriNetEditor-UnitTest
TimedPetriNetEditor-UnitTest.TYPE := test
TimedPetriNetEditor-UnitTest.DESCRIPTION := Unit tests for TimedPetriNetEditor
TimedPetriNetEditor-UnitTest.SOURCES := *.cpp
TimedPetriNetEditor-UnitTest.DEPENDS := TimedPetriAnalysis TimedPetriRuntime TimedPetriMaxPlus
TimedPetriNetEditor-UnitTest.PKG_CONFIG := gtest gmock
TimedPetriNetEditor-UnitTest.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion \
    -Wno-float-equal -fno-fast-math -Wno-nan-infinity-disabled \
    -Wno-character-conversion -Wno-keyword-macro
# The tests read their files in tests/data.
TimedPetriNetEditor-UnitTest.WORKDIR := .
