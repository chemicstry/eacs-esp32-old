ARDUINO_LIBS := $(patsubst $(COMPONENT_PATH)/%,%,$(sort $(dir $(wildcard $(COMPONENT_PATH)/*/src/))))

COMPONENT_ADD_INCLUDEDIRS := $(ARDUINO_LIBS)
COMPONENT_SRCDIRS := $(ARDUINO_LIBS)
CXXFLAGS += -fno-rtti -Wno-switch -Wno-narrowing
