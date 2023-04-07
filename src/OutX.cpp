#include "plugin.hpp"
#include "OutX.hpp"
// #include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"
#include "common.hpp"

using namespace constants;

OutX::OutX()
{

	auto setLeftLight = [this](float value)
	{
		lights[LIGHT_LEFT_CONNECTED].setBrightness(value);
	};
	auto setRightLight = [this](float value)
	{
		lights[LIGHT_RIGHT_CONNECTED].setBrightness(value);
	};

	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	this
		->addAllowedModel(modelSpike, LEFT)
		->setLeftLightOn(setLeftLight)
		->setRightLightOn(setRightLight);
}

void OutX::process(const ProcessArgs &args)
{
	if (leftExpander.module == NULL)
		for (int i = 0; i < NUM_CHANNELS; i++)
		{
			outputs[OUTPUT_SIGNAL + i].setVoltage(0.0f);
			outputs[OUTPUT_SIGNAL + i].setChannels(0);
		}
}

bool OutX::setOutput(const int outputIndex, const float value, const int channel, bool exclusive)
{
	if (normalledMode && exclusive)
	{
		outputs[lastHigh[channel]].setVoltage(0.f, channel);
		for (int i = outputIndex; i < 16; i++)
		{
			// XXX TODO Imperfect
			if (outputs[i].isConnected())
			{
				lastHigh[channel] = i;
				outputs[i].setVoltage(value, channel);
				return snoopMode;
			}
		}
	}
	else if (!normalledMode && exclusive)
	{
		// XXX TODO Imperfect
		if (channel != lastHigh[channel])
		{
			outputs[lastHigh[channel]].setVoltage(0.f, channel);
			lastHigh[channel] = outputIndex;
		}
		outputs[outputIndex].setVoltage(value, channel);
	}
	return false;
}

json_t *OutX::dataToJson()
{
	json_t *rootJ = json_object();
	json_object_set_new(rootJ, "snoopMode", json_boolean(snoopMode));
	json_object_set_new(rootJ, "normalledMode", json_boolean(normalledMode));
	// json_object_set_new(rootJ, "gateMode", json_integer(gateMode.gateMode));
	return rootJ;
}
void OutX::dataFromJson(json_t *rootJ)
{
	json_t *snoopModeJ = json_object_get(rootJ, "snoopMode");
	if (snoopModeJ)
		snoopMode = json_boolean_value(snoopModeJ);
	json_t *normalledModeJ = json_object_get(rootJ, "normalledMode");
	if (normalledModeJ)
		normalledMode = json_boolean_value(normalledModeJ);
	// json_t *gateModeJ = json_object_get(rootJ, "gateMode");
	// if (gateModeJ)
	// {
	// 	gateMode.setGateMode((GateMode::Modes)json_integer_value(gateModeJ));
	// };
};

Model *modelOutX = createModel<OutX, OutXWidget>("OutX");