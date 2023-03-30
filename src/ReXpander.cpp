#include "plugin.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"
#include "ReXpander.hpp"

using namespace constants;

ReXpander::ReXpander()
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(PARAM_START, 0.0f, 15.0f, 0.0f, "Start", "", 0.0f, 1.0f, 1.0f);
	configParam(PARAM_LENGTH, 1.0f, 16.0f, 16.0f, "Length");

	getParamQuantity(PARAM_START)->snapEnabled = true;
	getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
	configInput(INPUT_START, "Start CV");
	configInput(INPUT_LENGTH, "Length CV");

	auto setLeftLight = [this](float value)
	{
		lights[LIGHT_LEFT_CONNECTED].setBrightness(value);
	};
	auto setRightLight = [this](float value)
	{
		lights[LIGHT_RIGHT_CONNECTED].setBrightness(value);
	};
	this
	 	->addAllowedModel(modelInject, LEFT)
		->addAllowedModel(modelInject, RIGHT)
		->addAllowedModel(modelPhaseTrigg, RIGHT)
		->setLeftLightOn(setLeftLight)
		->setRightLightOn(setRightLight);
};

Model *modelReXpander = createModel<ReXpander, ReXpanderWidget>("ReX");