#include "plugin.hpp"
#include "InX.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"

using namespace constants;

InX::InX() : ModuleX({modelReX, modelInX}, {modelReX, modelSpike}, 
	[this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
	[this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

Model *modelInX = createModel<InX, InXWidget>("InX");