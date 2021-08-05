##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of PetriEditor.
##
## PetriEditor is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

TARGET_BIN = TimedPetriNetEditor
TARGET_LIB = libtpne.so

# make install
DESTDIR ?=
PREFIX ?= /usr
BINDIR := $(DESTDIR)$(PREFIX)/bin
LIBDIR := $(DESTDIR)$(PREFIX)/lib
DATADIR := $(DESTDIR)$(PREFIX)/share/$(TARGET_BIN)/data

# Search files
BUILD = build
VPATH = $(BUILD) src src/utils src/julia
INCLUDES = -Isrc

# Compilation options (c++14 only because of std::make_unique not present in C++11)
CXXFLAGS = --std=c++14 -W -Wall -Wextra -fPIC `pkg-config --cflags sfml-graphics`
LDFLAGS = `pkg-config --libs sfml-graphics` -lpthread
DEFINES = -DDATADIR=\"$(DATADIR)\"

# File dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD)/$*.Td
POSTCOMPILE = mv -f $(BUILD)/$*.Td $(BUILD)/$*.d

# Object files
LIB_OBJS = Howard.o PetriNet.o PetriEditor.o Julia.o
BIN_OBJS = $(LIB_OBJS) main.o

ifeq ($(VERBOSE),1)
Q :=
else
Q := @
endif

all: $(TARGET_BIN) $(TARGET_LIB)

# Link the target
$(TARGET_BIN): $(BIN_OBJS)
	@echo "Linking $@"
	$(Q)cd $(BUILD) && $(CXX) $(INCLUDES) -o $(TARGET_BIN) $(BIN_OBJS) $(LDFLAGS)

# Create the shared library
$(TARGET_LIB): $(LIB_OBJS)
	@echo "Library $@"
	$(Q)cd $(BUILD) && $(CXX) -shared -o $(TARGET_LIB) $(LIB_OBJS) $(LDFLAGS)

# Compile C++ source files
%.o : %.cpp $(BUILD)/%.d Makefile
	@echo "Compiling $<"
	$(Q)$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $(abspath $<) -o $(abspath $(BUILD)/$@)
	@$(POSTCOMPILE)

# Compile C source files
%.o : %.c $(BUILD)/%.d Makefile
	@echo "Compiling $<"
	$(Q)$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $(abspath $<) -o $(abspath $(BUILD)/$@)
	@$(POSTCOMPILE)

# Install
install: $(TARGET_BIN) $(TARGET_LIB)
	@echo "Installing $(TARGET_BIN)"
	$(Q)mkdir -p $(BINDIR)
	$(Q)mkdir -p $(LIBDIR)
	$(Q)mkdir -p $(DATADIR)
	cp $(BUILD)/$(TARGET_BIN) $(BINDIR)
	cp $(BUILD)/$(TARGET_LIB) $(LIBDIR)
	cp -r data $(DATADIR)/..

# Delete compiled files
.PHONY: clean
clean:
	-rm -fr $(BUILD)

# Create the directory before compiling sources
$(BIN_OBJS): | $(BUILD)
$(BUILD):
	@mkdir -p $(BUILD)

# Create the dependency files
$(BUILD)/%.d: ;
.PRECIOUS: $(BUILD)/%.d

-include $(patsubst %,$(BUILD)/%.d,$(basename $(BIN_OBJS)))
