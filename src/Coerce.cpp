#include "plugin.hpp"
#include "components.hpp"
#include <array>
#include <algorithm>
#include <functional>

struct Coerce : Module
{

	enum ParamId
	{
		CONTROL_RATE,
		PARAMS_LEN
	};
	enum InputId
	{
		SELECTIONS1_INPUT,
		SELECTIONS2_INPUT,
		SELECTIONS3_INPUT,
		SELECTIONS4_INPUT,
		SELECTIONS5_INPUT,
		SELECTIONS6_INPUT,
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		IN4_INPUT,
		IN5_INPUT,
		IN6_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		OUT5_OUTPUT,
		OUT6_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		LIGHTS_LEN
	};
	int const INPUTS_AND_OUTPUTS = 6;

	enum class RestrictMethod
	{
		RESTRICT,	 // Only allow values from the selection
		OCTAVE_FOLD, // Allow values from the selection, but fold them into their own octave
	};

	enum class RoundingMethod
	{
		CLOSEST,
		DOWN,
		UP
	};

	enum class ControlRate
	{
		AUDIO_RATE = 1,
		CONTROL_RATE = 50 // XXX Make relative to sample rate.
	};

	RestrictMethod restrictMethod = RestrictMethod::OCTAVE_FOLD;
	RoundingMethod roundingMethod = RoundingMethod::CLOSEST;
	ControlRate controlRate = ControlRate::CONTROL_RATE;

	int controlRateCounter = (int)controlRate;

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "restrictMethod", json_integer((int)restrictMethod));
		json_object_set_new(rootJ, "roundingMethod", json_integer((int)roundingMethod));
		json_object_set_new(rootJ, "controlRate", json_integer((int)controlRate));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override
	{
		json_t *restrictMethodJ = json_object_get(rootJ, "restrictMethod");
		if (restrictMethodJ)
			restrictMethod = (RestrictMethod)json_integer_value(restrictMethodJ);

		json_t *roundingMethodJ = json_object_get(rootJ, "roundingMethod");
		if (roundingMethodJ)
			roundingMethod = (RoundingMethod)json_integer_value(roundingMethodJ);

		json_t *controlRateJ = json_object_get(rootJ, "controlRate");
		if (controlRateJ)
			controlRate = (ControlRate)json_integer_value(controlRateJ);
	}

	Coerce()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(IN1_INPUT, "1");
		configInput(IN2_INPUT, "2");
		configInput(IN3_INPUT, "3");
		configInput(IN4_INPUT, "4");
		configInput(IN5_INPUT, "5");
		configInput(IN6_INPUT, "6");
		configInput(SELECTIONS1_INPUT, "Quantize 1");
		configInput(SELECTIONS2_INPUT, "Quantize 2");
		configInput(SELECTIONS3_INPUT, "Quantize 3");
		configInput(SELECTIONS4_INPUT, "Quantize 4");
		configInput(SELECTIONS5_INPUT, "Quantize 5");
		configInput(SELECTIONS6_INPUT, "Quantize 6");
		configOutput(OUT1_OUTPUT, "1");
		configOutput(OUT1_OUTPUT, "2");
		configOutput(OUT1_OUTPUT, "3");
		configOutput(OUT1_OUTPUT, "4");
		configOutput(OUT1_OUTPUT, "5");
		configOutput(OUT1_OUTPUT, "6");
	}

	void adjustValues(int quantizeInputId, float input[], float output[], int inputLen, float quantize[], int selectionsLen)
	{
		switch (restrictMethod)
		{
		case RestrictMethod::RESTRICT:
		{
			for (int i = 0; i < inputLen; i++)
			{
				float value = input[i];
				float closest = NAN;
				float biggest = -1000000.0f;
				float smallest = 1000000.0f;
				float min_diff = 1000000.0f;

				for (int j = 0; j < selectionsLen; j++)
				{
					float diff = fabsf(value - quantize[j]);
					if (roundingMethod == RoundingMethod::CLOSEST)
					{
						if (diff < min_diff)
						{
							closest = quantize[j];
							min_diff = diff;
						}
					}
					else
					{
						if (roundingMethod == RoundingMethod::DOWN)
						{
							if (quantize[j] < smallest)
							{
								smallest = quantize[j];
							}
							if ((quantize[j] <= value) && (diff < min_diff))
							{
								closest = quantize[j];
								min_diff = diff;
							}
						}
						else if (roundingMethod == RoundingMethod::UP)
						{

							if (quantize[j] >= biggest)
							{
								biggest = quantize[j];
							}
							if ((quantize[j] >= value) && (diff < min_diff))
							{
								closest = quantize[j];
								min_diff = diff;
							}
						}
					}
				}
				if ((roundingMethod == RoundingMethod::DOWN) && (value < smallest))
				{
					closest = smallest;
				}
				else if ((roundingMethod == RoundingMethod::UP) && (value > biggest))
				{
					closest = biggest;
				}
				output[i] = closest;
			}
			break;
		}
		case RestrictMethod::OCTAVE_FOLD:
		{
			auto roundingCondition = [](float x, RoundingMethod roundMethod) -> bool
			{
				switch (roundMethod)
				{
				case RoundingMethod::CLOSEST:
					return true;
				case RoundingMethod::DOWN:
					return (x >= 0);
				case RoundingMethod::UP:
					return (x <= 0);
				}
				return false;
			};
			for (int i = 0; i < inputLen; i++)
			{
				float min_diff = 1000000.0f;
				int closestIdx = -1;
				int addOctave = 0;
				float inputFraction = (input[i] - floor(input[i]));
				for (int j = 0; j < selectionsLen; j++)
				{
					const float quantizeFraction = quantize[j] - floor(quantize[j]);
					const float diff = inputFraction - quantizeFraction;
					const float diff2 = inputFraction - (quantizeFraction - 1.0f);
					const float diff3 = inputFraction - (quantizeFraction + 1.0f);

					if ((abs(diff) < abs(min_diff)) || (abs(diff2) < abs(min_diff)) || (abs(diff3) < abs(min_diff)))
					{
						if ((abs(diff) < abs(min_diff)) && roundingCondition(diff, roundingMethod))
						{
							closestIdx = j;
							min_diff = diff;
							addOctave = 0;
						}
						else if ((abs(diff2) < abs(min_diff)) && roundingCondition(diff2, roundingMethod))
						{
							closestIdx = j;
							min_diff = diff2;
							addOctave = -1;
						}
						else if ((abs(diff3) < abs(min_diff)) && roundingCondition(diff3, roundingMethod))
						{
							closestIdx = j;
							min_diff = diff3;
							addOctave = 1;
						}
					}
				}
				output[i] = floor(input[i]) + addOctave + abs(quantize[closestIdx] - floor(quantize[closestIdx]));
			}
			break;
		}

		default:
			break;
		}
	}

	bool getNormalledSelections(const int input_idx, int &sel_count, int &sel_id, float *selections)
	{

		for (int i = input_idx; i >= 0; i--)
		{
			if (inputs[SELECTIONS1_INPUT + i].isConnected())
			{
				sel_count = inputs[SELECTIONS1_INPUT + i].getChannels();
				sel_id = SELECTIONS1_INPUT + i;

				inputs[SELECTIONS1_INPUT + i].readVoltages(selections);
				return true;
			}
		}
		sel_count = 0;
		sel_id = 0;
		return false;
	}

	void onPortChange(const PortChangeEvent &e) override
	{
		// Reset output channels when input is disconnected
		if (!e.connecting && (e.type == Port::Type::INPUT))
		{
			// Input disconnect
			if (e.portId >= IN1_INPUT && e.portId <= IN1_INPUT + INPUTS_AND_OUTPUTS)
			{
				int input_idx = e.portId - IN1_INPUT;
				if (outputs[OUT1_OUTPUT + input_idx].isConnected())
				{
					outputs[OUT1_OUTPUT + input_idx].setChannels(inputs[IN1_INPUT + input_idx].getChannels());
				}
			}
		}
	};

	void process(const ProcessArgs &args) override
	{
		if (controlRateCounter != 0)
		{
			controlRateCounter--;
			return;
		}
		else
		{
			controlRateCounter = (int)controlRate;
		}
		for (int i = 0; i < INPUTS_AND_OUTPUTS; i++)
		{
			if (inputs[IN1_INPUT + i].isConnected() && outputs[OUT1_OUTPUT + i].isConnected())
			{
				int sel_count = 0;
				int sel_id = 0;
				float selections[16] = {};
				if (getNormalledSelections(i, sel_count, sel_id, selections))
				{
					outputs[OUT1_OUTPUT + i].setChannels(inputs[IN1_INPUT + i].getChannels());
					adjustValues(
						sel_id,
						inputs[IN1_INPUT + i].getVoltages(),
						outputs[OUT1_OUTPUT + i].getVoltages(),
						inputs[IN1_INPUT + i].getChannels(),
						selections,
						sel_count);
				}
			}
		}
	}
};

struct CoerceWidget : ModuleWidget
{
	CoerceWidget(Coerce *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Coerce.svg")));

		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 30.0)), module, Coerce::IN1_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 40.0)), module, Coerce::IN2_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 50.0)), module, Coerce::IN3_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 60.0)), module, Coerce::IN4_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 70.0)), module, Coerce::IN5_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(5.08, 80.0)), module, Coerce::IN6_INPUT));

		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 30.0)), module, Coerce::SELECTIONS1_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 40.0)), module, Coerce::SELECTIONS2_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 50.0)), module, Coerce::SELECTIONS3_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 60.0)), module, Coerce::SELECTIONS4_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 70.0)), module, Coerce::SELECTIONS5_INPUT));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(15.24, 80.0)), module, Coerce::SELECTIONS6_INPUT));

		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 30.0)), module, Coerce::OUT1_OUTPUT));
		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 40.3)), module, Coerce::OUT2_OUTPUT));
		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 50.0)), module, Coerce::OUT3_OUTPUT));
		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 60.0)), module, Coerce::OUT4_OUTPUT));
		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 70.0)), module, Coerce::OUT5_OUTPUT));
		addOutput(createOutputCentered<SmallPort>(mm2px(Vec(25.48, 80.0)), module, Coerce::OUT6_OUTPUT));
	};
	void appendContextMenu(Menu *menu) override
	{
		Coerce *module = dynamic_cast<Coerce *>(this->module);
		assert(module);

		struct RestrictMethodMenuItem : MenuItem
		{
			Coerce *module;
			Coerce::RestrictMethod method;
			void onAction(const event::Action &e) override
			{
				module->restrictMethod = method;
			}
			void step() override
			{
				rightText = (module->restrictMethod == method) ? "✔" : "";
				MenuItem::step();
			}
			RestrictMethodMenuItem(std::string label, Coerce::RestrictMethod method, Coerce *module)
			{
				this->text = label;
				this->method = method;
				this->module = module;
			}
		};

		struct RoundingMethodMenuItem : MenuItem
		{
			Coerce *module;
			Coerce::RoundingMethod method;
			void onAction(const event::Action &e) override
			{
				module->roundingMethod = method;
			}
			void step() override
			{
				rightText = (module->roundingMethod == method) ? "✔" : "";
				MenuItem::step();
			}
			RoundingMethodMenuItem(std::string label, Coerce::RoundingMethod method, Coerce *module)
			{
				this->text = label;
				this->method = method;
				this->module = module;
			}
		};

		struct ControlRateMenuItem : MenuItem
		{
			Coerce *module;
			Coerce::ControlRate controlRate;
			void onAction(const event::Action &e) override
			{
				module->controlRate = controlRate;
			}
			void step() override
			{
				rightText = (module->controlRate == controlRate) ? "✔" : "";
				MenuItem::step();
			}
			ControlRateMenuItem(std::string label, Coerce::ControlRate controlRate, Coerce *module)
			{
				this->text = label;
				this->controlRate = controlRate;
				this->module = module;
			}
		};

		menu->addChild(new MenuSeparator);
		menu->addChild(new RestrictMethodMenuItem{"Octave Fold", Coerce::RestrictMethod::OCTAVE_FOLD, module});
		menu->addChild(new RestrictMethodMenuItem{"Restrict", Coerce::RestrictMethod::RESTRICT, module});
		menu->addChild(new MenuSeparator);
		menu->addChild(new RoundingMethodMenuItem{"Up", Coerce::RoundingMethod::UP, module});
		menu->addChild(new RoundingMethodMenuItem{"Closest", Coerce::RoundingMethod::CLOSEST, module});
		menu->addChild(new RoundingMethodMenuItem{"Down", Coerce::RoundingMethod::DOWN, module});
		menu->addChild(new MenuSeparator);
		menu->addChild(new ControlRateMenuItem{"Audio Rate", Coerce::ControlRate::AUDIO_RATE, module});
		menu->addChild(new ControlRateMenuItem{"Control Rate", Coerce::ControlRate::CONTROL_RATE, module});
	}
};

Model *modelCoerce = createModel<Coerce, CoerceWidget>("Coerce");