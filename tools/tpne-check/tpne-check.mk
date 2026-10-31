# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# Check the properties of Petri nets in a terminal or a CI.

TARGETS += tpne-check
tpne-check.DESCRIPTION := Check the properties of Petri nets in a terminal or a CI
tpne-check.SOURCES := main.cpp
tpne-check.DEPENDS := TimedPetriAnalysis TimedPetriIO
tpne-check.LIBS := -lpthread
