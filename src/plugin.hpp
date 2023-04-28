#pragma once
#include <rack.hpp>

using namespace rack; // NOLINT

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance; // NOLINT

// Declare each Model, defined in each module source file
extern Model *modelBlank;   // NOLINT
extern Model *modelArray;   // NOLINT
extern Model *modelThru;         // NOLINT
extern Model *modelCoerce;  // NOLINT
extern Model *modelCoerce6; // NOLINT
extern Model *modelSpike;   // NOLINT
extern Model *modelReX;     // NOLINT
extern Model *modelInX;     // NOLINT
extern Model *modelOutX;    // NOLINT
