#pragma once
#include <rack.hpp>

using namespace rack;  // NOLINT

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;  // NOLINT

// Declare each Model, defined in each module source file
extern Model* modelBlank;    // NOLINT
extern Model* modelPhi;      // NOLINT
extern Model* modelArr;      // NOLINT
extern Model* modelVia;      // NOLINT
extern Model* modelCoerce;   // NOLINT
extern Model* modelCoerce6;  // NOLINT
extern Model* modelSpike;    // NOLINT
extern Model* modelReX;      // NOLINT
extern Model* modelInX;      // NOLINT
extern Model* modelOutX;     // NOLINT
extern Model* modelTie;      // NOLINT
extern Model* modelBank;     // NOLINT
extern Model* modelModX;     // NOLINT
extern Model* modelOpX;      // NOLINT
extern Model* modelAlgoX;    // NOLINT
extern Model* modelGaitX;    // NOLINT
