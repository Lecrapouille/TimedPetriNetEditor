# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# Third parties of the libraries, fetched on the first build at the commits
# of external/versions.lock (commit it). Those of the editor are in gui.mk.

TARGETS += json tinyxml2

### JSON for Modern C++. License: MIT
json.GIT := https://github.com/nlohmann/json.git
json.PUBLIC_INCLUDES := include

### TinyXML2 is a simple, small, efficient, C++ XML parser. License: Zlib
tinyxml2.GIT := https://github.com/leethomason/tinyxml2.git
tinyxml2.TYPE := static
tinyxml2.SOURCES := tinyxml2.cpp
tinyxml2.PUBLIC_INCLUDES := .
