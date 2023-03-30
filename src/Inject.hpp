
#include "plugin.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "components.hpp"

using namespace constants;

struct Inject : ModuleX
{
public:
	enum ParamId
	{
		PARAMS_LEN
	};
	enum InputId
	{
		ENUMS(INPUT_SIGNAL, 16),
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

	// bool dirty = true;

public:
	Inject();
};

struct InjectWidget : ModuleWidget
{
	const float width = 4 * HP;
	float centre = width / 2.0f;
	InjectWidget(Inject *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Inject.svg")));

		// XXX TODO move two connection lights to ModuleX
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, Inject::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, Inject::LIGHT_RIGHT_CONNECTED));

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				addInput(createInputCentered<SmallPort>(mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module, Inject::INPUT_SIGNAL + (i * 8) + j));
			}
		}
	}
};
