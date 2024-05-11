# If RACK_DIR is not defined when calling the Makefile, default to two directories above
ifndef RACK_DIR
RACK_DIR ?= ../..
endif
+FLAGS += -march=nocona -ffast-math -fno-finite-math-only
# FLAGS will be passed to both the C and C++ compiler
FLAGS += 
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)


# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS += 

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/sp/*.cpp)
SOURCES += $(wildcard src/comp/*.cpp)
SOURCES += $(wildcard src/helpers/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk


CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS))
CXXFLAGS += -std=c++20