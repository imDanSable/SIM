#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelBlank;
extern Model* modelCoerce;
extern Model* modelCoerce6;
extern Model* modelSpike;
extern Model* modelReX;
extern Model* modelInX;
extern Model* modelOutX;

// Provide allowed lists for all models
// XXX TODO this doesn't work. So now in spike I use these and the contstructors ard double code {...,...}
// someone advice to use pointers 
// std::vector<rack::plugin::Model*>* rexRightAllowedModels = nullptr;

// ...

// void initRexRightAllowedModels() {
//   rexRightAllowedModels = new std::vector<rack::plugin::Model*> {modelInX, modelSpike};
// }


// OR:

// []() -> const std::vector<rack::plugin::Model*>& {
//   static std::vector<rack::plugin::Model*> rexRightAllowedModels = {modelInX, modelSpike};
//   return rexRightAllowedModels;
// }(),.....

// ...


const std::vector<rack::plugin::Model *> rexLeftAllowedModels = {modelInX};
const std::vector<rack::plugin::Model *> rexRightAllowedModels = {modelSpike, modelInX};
const std::vector<rack::plugin::Model *> inxLeftAllowedModels = {modelReX};
const std::vector<rack::plugin::Model *> inxRightAllowedModels = {modelReX, modelSpike};
const std::vector<rack::plugin::Model *> spikeLeftAllowedModels = {modelReX, modelInX};
const std::vector<rack::plugin::Model *> spikeRightAllowedModels = {modelOutX};
const std::vector<rack::plugin::Model *> outxLeftAllowedModels = {modelSpike};
const std::vector<rack::plugin::Model *> outxRightAllowedModels = {};