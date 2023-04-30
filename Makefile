# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..


# FLAGS will be passed to both the C and C++ compiler
FLAGS += 
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS += 

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk

ifdef NDEBUG
  CXXFLAGS := $(filter-out -fno-omit-frame-pointer,$(CXXFLAGS))
  CXXFLAGS := $(filter-out -funsafe-math-optimizations,$(CXXFLAGS))
  CXXFLAGS := $(filter-out -O3,$(CXXFLAGS))
  CXXFLAGS += -O0 -g -DNDEBUG
  CFLAGS += -O0 -g -DNDEBUG
endif

ifdef USE_CLANG
    CC = clang
    CXX = clang++
    CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
endif

ifdef BEAR
CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
endif

# CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS))
# CXXFLAGS += -std=c++17