#include "plugin.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"
#include "Rex.hpp"

using namespace constants;

ReX::ReX() : ModuleX({modelInX}, {modelInX, modelSpike},
	[this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
	[this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(PARAM_START, 0.0f, 15.0f, 0.0f, "Start", "", 0.0f, 1.0f, 1.0f);
	configParam(PARAM_LENGTH, 1.0f, 16.0f, 16.0f, "Length");

	getParamQuantity(PARAM_START)->snapEnabled = true;
	getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
	configInput(INPUT_START, "Start CV");
	configInput(INPUT_LENGTH, "Length CV");
};

Model *modelReX = createModel<ReX, ReXWidget>("ReX");
