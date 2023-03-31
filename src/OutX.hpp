
#include "plugin.hpp"
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
using namespace constants;
struct OutX : ModuleX
{
	enum ParamId
	{
		PARAMS_LEN
	};
	enum InputId
	{
		INPUTS_LEN
	};
	enum OutputId
	{
		ENUMS(OUTPUT_SIGNAL, 16),
		OUTPUTS_LEN
	};
	enum LightId
	{
		LIGHT_LEFT_CONNECTED,
		LIGHT_RIGHT_CONNECTED,
		LIGHTS_LEN
	};

	bool normalledMode = false;
	bool snoopMode = false;

	OutX();
	void process(const ProcessArgs &args) override;
	json_t * dataToJson() override;
	void dataFromJson(json_t *rootJ) override;
};

struct OutXWidget : ModuleWidget
{
	OutXWidget(OutX *module)
	{
		const auto width = 4 * HP;
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/OutX.svg")));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, OutX::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(width - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, OutX::LIGHT_RIGHT_CONNECTED));
		// addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(HP / 4.0f, Y_POSITION_CONNECT_LIGHT)), module, OutX::LIGHT_CONNECTED));
		int id = 0;
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				addOutput(createOutputCentered<SIMPort>(mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module, id++));
			}
		}
		// addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, 100)), module, OutX::PARAM_DURATION));
		// addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 110)), module, OutX::INPUT_DURATION));
	}

	void appendContextMenu(Menu *menu) override
	{
		OutX *module = dynamic_cast<OutX *>(this->module);
		assert(module);
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Normalled Mode", "", &module->normalledMode));
		menu->addChild(createBoolPtrMenuItem("Snoop Mode", "", &module->snoopMode));
		// menu->addChild(module->gateMode.createMenuItem());
	}
};
