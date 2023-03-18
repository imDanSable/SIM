#pragma once
#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"

using namespace constants;

struct ReXpander : public ModuleX
{
	enum ParamId
	{
		PARAM_START,
		PARAM_LENGTH,
		PARAMS_LEN
	};
	enum InputId
	{
		INPUT_START,
		INPUT_LENGTH,
		INPUTS_LEN
	};
	enum OutputId
	{
		OUTPUTS_LEN
	};
	enum LightId
	{
		LIGHT_LEFT_CONNECTED,
		LIGHT_RIGHT_CONNECTED,
		LIGHTS_LEN
	};

	ReXpander();
};


struct ReXpanderWidget : ModuleWidget
{
	ReXpanderWidget(ReXpander *module)
	{
		const float center = 1.f * HP;
		const float width = 2.f * HP;
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/ReXpander.svg")));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, ReXpander::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(width - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, ReXpander::LIGHT_RIGHT_CONNECTED));

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT)), module, ReXpander::PARAM_START));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT + JACKYSPACE)), module, ReXpander::INPUT_START));

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT)), module, ReXpander::PARAM_LENGTH));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT + JACKYSPACE)), module, ReXpander::INPUT_LENGTH));

	}
};
