PROJECT = TimedPetriNetEditor
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
USE_COVERAGE = 1

P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P) $(P)/src $(P)/src/utils
INCLUDES += -I$(P) -I$(P)/src

OBJS += PetriNet.o Howard.o
OBJS += PetriNetTests.o
OBJS += main.o

PKG_LIBS += gtest gmock

all: $(TARGET)

.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) $(TU_OPTIONS) || (cat $(TARGET).log; return 1)

include $(M)/Makefile.footer
