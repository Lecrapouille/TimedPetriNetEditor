# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

PROJECT_NAME := TimedPetriNetEditor
PROJECT_VERSION := 0.8.0
COMPILATION_MODE ?= debug
CXX_STANDARD := c++20

###############################################################################
# The libraries (TimedPetriNet.mk, also read by the projects embedding
# them), their third parties, the editor, the library for Julia, the tools
# and the tests, each declared in the directory of its sources.
#
SUBMAKEFILES := TimedPetriNet.mk external/external.mk external/gui.mk \
    src/Editor/Editor.mk src/julia/julia.mk tools/tpne-check/tpne-check.mk tests/tests.mk

M ?= .makefile
include $(M)/MyMakefile.mk
