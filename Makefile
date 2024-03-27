# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..


# FLAGS will be passed to both the C and C++ compiler
+FLAGS += -march=nocona -ffast-math -fno-finite-math-only

#extra flags required by gammin
FLAGS += -I Gammin
FLAGS += -D__STDC_CONSTANT_MACROS
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)


# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS += 

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/DSP/*.cpp)
SOURCES += $(wildcard src/DSP/Phasors/*.cpp)
SOURCES += Gammin/src/arr.cpp
SOURCES += Gammin/src/Domain.cpp
SOURCES += Gammin/src/scl.cpp



# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk

ifdef DEBUG
  CXXFLAGS := $(filter-out -fno-omit-frame-pointer,$(CXXFLAGS))
  CXXFLAGS := $(filter-out -funsafe-math-optimizations,$(CXXFLAGS))
  CXXFLAGS := $(filter-out -O3,$(CXXFLAGS))
  CXXFLAGS += -O0 -g
  CFLAGS += -O0 -g
else
  CXXFLAGS += -DNDEBUG
  CFLAGS += -DNDEBUG
endif

ifdef USE_ASAN
  CFLAGS += -fsanitize=address
  CXXFLAGS += -fsanitize=address
  LDFLAGS += -fsanitize=address
  LDFLAGS += -Wl,-rpath=/usr/lib/llvm-6.0/lib
  LD_PRELOAD += /lib/x86_64-linux-gnu/libasan.so.6 /home/b/dev/Rack/Rack
endif

ifdef USE_GPROF
  CFLAGS += -pg
  CXXFLAGS += -pg
  LDFLAGS += -pg
endif

ifdef USE_CLANG
    CC = clang
    CXX = clang++
    CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
endif

ifdef KEEP_SYMBOLS
  CXXFLAGS := $(filter-out -O3,$(CXXFLAGS))
  CFLAGS := $(filter-out -O3,$(CXXFLAGS))
  CFLAGS += -g3
  CXXFLAGS += -g3
  CXXFLAGS += -O2
  CFLAGS += -O2
  CXXFLAGS += -fno-inline
endif

ifdef BEAR
CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
endif

CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS))
# CXXFLAGS += -std=c++17
CXXFLAGS += -std=c++20