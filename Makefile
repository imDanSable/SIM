# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
# FLAGS += -O3
# FLAGS += -O0 -g -DDEBUGGING
# FLAGS += -I./dep/variant/include
FLAGS += 
CFLAGS += $(FLAGS)
CXXFLAGS += $(FLAGS)

ifdef NDEBUG
  CXXFLAGS += -DNDEBUG
endif

ifdef NDEBUG
	FLAGS += -O0 -g -DNDEBUG
else
	FLAGS += -O3 -funsafe-math-optimizations -fno-omit-frame-pointer
endif

ifdef USE_CLANG
    CC = clang
    CXX = clang++
    CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
endif
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

# Include the Rack plugin Makefile framework
# ifdef USE_CLANG
#     include $(RACK_DIR)/clang-plugin.mk	
# else
include $(RACK_DIR)/plugin.mk
# endif

