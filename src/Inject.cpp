#include "plugin.hpp"
#include "Inject.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"

using namespace constants;

Inject::Inject()
{
	this
		->addAllowedModel(modelReXpander, LEFT)
		->addAllowedModel(modelReXpander, RIGHT)
		->addAllowedModel(modelPhaseTrigg, RIGHT)
		->setLeftLightOn([this](float value)
						 { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); })
		->setRightLightOn([this](float value)
						  { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); });
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

Model *modelInject = createModel<Inject, InjectWidget>("InX");