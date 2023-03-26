#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"
#include "GateMode.hpp"
#include "ReXpander.hpp"

#ifdef DEBUGGING
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sstream>
#endif

using namespace constants;

struct PhaseTrigg : ModuleX
{
	enum ParamId
	{
		ENUMS(PARAM_GATE, 16),
		PARAM_DURATION,
		PARAMS_LEN
	};
	enum InputId
	{
		INPUT_CV,
		INPUT_GATE_PATTERN,
		INPUT_DURATION_CV,
		INPUTS_LEN
	};
	enum OutputId
	{
		OUTPUT_GATE,
		OUTPUTS_LEN
	};
	enum LightId
	{
		ENUMS(LIGHTS_GATE, 16 * 3),
		ENUMS(LIGHTS_ACTIVE_GATE, 16),
		LIGHT_LEFT_CONNECTED,
		LIGHT_RIGHT_CONNECTED,
		LIGHTS_LEN
	};

	GateMode gateMode;
	int monitorChannel = 0;

private:
	dsp::Timer uiTimer;
	float prevCv[NUM_CHANNELS] = {};
	int prevChannelIndex[NUM_CHANNELS] = {};
	dsp::PulseGenerator triggers[NUM_CHANNELS];

	/* Expander stuff */

	//@return true if direction is forward
	inline bool getDirection(float cv, float prevCv)
	{
		const float diff = cv - prevCv;
		if (((diff >= 0) && (diff <= 5.f)) || ((-5.f >= diff) && (diff <= 0)))
			return true;
		else
			return false;
	};

	inline void setLedColor(int index, float r, float g, float b)
	{

		lights[0 + index * 3].setBrightness(clamp(r));
		lights[1 + index * 3].setBrightness(clamp(g));
		lights[2 + index * 3].setBrightness(clamp(b));
	}

	inline void setLedColor(int index, const Color &color)
	{
		setLedColor(index, color.r, color.g, color.b);
	}

public:
	PhaseTrigg() : gateMode(this, PARAM_DURATION)
	{
		this
			->addAllowedModel(modelReXpander, LEFT)
			->setLeftLightOn([this](float value)
							 { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); })
			->setRightLightOn([this](float value)
							  { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); });
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_CV, "Φ-in");
		configInput(INPUT_GATE_PATTERN, "Gate pattern input");
		configInput(INPUT_DURATION_CV, "Duration CV");
		configOutput(OUTPUT_GATE, "Trigger/Gate");
		configParam(PARAM_DURATION, 0.1f, 100.f, 0.1f, "Gate duration", "%", 0, 1.f);

		for (int i = 0; i < 16; i++)
			configParam(PARAM_GATE + i, 0.0f, 1.0f, 0.0f, "Gate " + std::to_string(i + 1));
	}

	void onReset() override
	{
		for (int i = 0; i < NUM_CHANNELS; i++)
		{
			prevCv[i] = 0.f;
			prevChannelIndex[i] = 0;
		}
	}
	/// @brief 
	/// @param start (index) so 0 to num_lights - 1
	/// @param length (count) so 1 to num_lights
	/// @param num_lights  0 to 16
	void updateUi(const int start, const int length, const int num_lights)
	{
		for (int i = 0; i < 16; i++) // reset all lights
		{
			// black
			setLedColor(LIGHTS_GATE + i, BLACK);
			lights[LIGHTS_ACTIVE_GATE + i].setBrightness(0.f);
		}

		bool light_on;

		// Out of range before start
		for (int i = 0; i < start; i++)
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % num_lights)) > 0.f; // % not tested in_start/in_length
			else
				light_on = params[(PARAM_GATE + i) % num_lights].getValue() > 0.f;
			if (light_on)
				// gray
				setLedColor((LIGHTS_GATE + i) % num_lights, GRAY);
		}
		// Start light
		// green
		setLedColor(LIGHTS_GATE + (start % num_lights), GREEN);

		// Active lights
		for (int i = start + 1; i < start + length; i++) // in range
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % num_lights)) > 0.f; // % not tested in_start/in_length
			else
				light_on = params[(PARAM_GATE + i) % num_lights].getValue() > 0.f;

			if (light_on)
				// white
				setLedColor((LIGHTS_GATE + i) % num_lights, WHITE);
		}

		// End light
		// red
		// int endLightIndex = (start + length - 1) % num_lights;
		int endLightIndex = (start + length - 1) % num_lights;
		WARN("endLightIndex: %d, start: %d, length: %d", endLightIndex, start, length);
		setLedColor(LIGHTS_GATE + endLightIndex, RED);

		// Out of active range after end
		for (int i = start + length; i < num_lights; i++)
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % num_lights)) > 0.f; // % not tested in_start/in_length
			else
				light_on = params[(PARAM_GATE + i) % num_lights].getValue() > 0.f;
			if (light_on)
				// gray
				setLedColor((LIGHTS_GATE + i) % num_lights, GRAY);
		}
	}

	void process(const ProcessArgs &args) override
	{
		ModuleX::process(args);
		ReXpander *rex = dynamic_cast<ReXpander *>(getModel(modelReXpander, LEFT));
		const bool rex_start_cv_connected = rex && rex->inputs[ReXpander::INPUT_START].isConnected();
		const bool rex_length_cv_connected = rex && rex->inputs[ReXpander::INPUT_LENGTH].isConnected();
		const int phaseChannelCount = inputs[INPUT_CV].getChannels();
		outputs[OUTPUT_GATE].setChannels(phaseChannelCount);
		const bool ui_update = uiTimer.process(args.sampleTime) > UI_UPDATE_TIME;
		const int patternLength = inputs[INPUT_GATE_PATTERN].getChannels();
		// clampLimit between 1 and zero
		const float clampLimit = patternLength > 0 ? patternLength : 16.f;

		// Start runs from 0 to 15 (index)
		int start = 0;
		// Length runs from 1 to 16 (count) and never zero
		int length = 16;

		auto calc_start_and_length = [&rex, &patternLength, &rex_start_cv_connected, &rex_length_cv_connected, &clampLimit, &start, &length](int channel)
		{
			if (rex)
			{
				const float rex_start_cv_input = rex->inputs[ReXpander::INPUT_START].getNormalPolyVoltage(0, channel);
				const float rex_length_cv_input = rex->inputs[ReXpander::INPUT_LENGTH].getNormalPolyVoltage(0, channel);
				const float rex_param_start = rex->params[ReXpander::PARAM_START].getValue();
				const float rex_param_length = rex->params[ReXpander::PARAM_LENGTH].getValue();
				if (patternLength == 0)
				{
					start = rex_start_cv_connected ?
						clamp(rescale(rex_start_cv_input, 0.f, 10.f, 0.f, clampLimit), 0.f, clampLimit - 1) : 
						rex_param_start;
					length = rex_length_cv_connected ?
						clamp(rescale(rex_length_cv_input, 0.f, 10.f, 1.f, clampLimit+1), 1.f, clampLimit) : 
						rex_param_length;
				}
				else
				{
					start = rex_start_cv_connected ? 
						clamp(rescale(rex_start_cv_input, 0.f, 10.f, 0.f, clampLimit), 0.f, clampLimit - 1) : 
						clamp(rex_param_start, 0.f, clampLimit - 1);
					length = rex_length_cv_connected ? 
						clamp(rescale(rex_length_cv_input, 0.f, 10.f, 1.f, clampLimit+1), 1.f, clampLimit) : 
						clamp(rex_param_length, 0.f, clampLimit);
				}
			}
		};

		if (ui_update)
		{
			uiTimer.reset();
			if (phaseChannelCount == 0) // no input connected. Update
			{
				calc_start_and_length(0);
				updateUi(start, length, clampLimit);
			}
		}
		for (int phaseChannel = 0; phaseChannel < phaseChannelCount; phaseChannel++)
		{

			calc_start_and_length(phaseChannel);

			const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, phaseChannel);
			const bool change = (cv != prevCv[phaseChannel]);
			const float phase = clamp(cv / 10.f, 0.f, .996f);
			const bool direction = getDirection(cv, prevCv[phaseChannel]);
			prevCv[phaseChannel] = cv;
			const int channel_index = int(((clamp(int(floor(length * (phase))), 0, length)) + start)) % int(clampLimit);

			if ((phaseChannel == monitorChannel) && ui_update)
				updateUi(start, length, clampLimit);

			// Set active gate light
			if (ui_update && (phaseChannel == monitorChannel))
			{
				lights[LIGHTS_ACTIVE_GATE + channel_index].setBrightness(1.f);
				if (phaseChannel == monitorChannel)
					setLedColor(LIGHTS_GATE + channel_index, 0.f, 0.f, .5f);
			}

			if ((prevChannelIndex[phaseChannel] != (channel_index)) && change) // change bool to assure we don't trigger if ReXpander is modifying us
			{
				// XXX MAYBE we should see if anything between prevChannelIndex +/- 1
				// and channel_index is set so we can trigger those gates too
				// when the input signal jumps

				if (((inputs[INPUT_GATE_PATTERN].getChannels() > 0) && inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, channel_index) > 0) || ((patternLength == 0) && (params[PARAM_GATE + channel_index].getValue() > 0.f)))
				{
					const float adjusted_duration = params[PARAM_DURATION].getValue() * 0.1f * inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.f, channel_index);
					gateMode.triggerGate(phaseChannel, adjusted_duration, phase, length, direction);
				}
				prevChannelIndex[phaseChannel] = channel_index;
			}
			if (gateMode.process(phaseChannel, phase, args.sampleTime))
			{
				outputs[OUTPUT_GATE].setVoltage(10.f, phaseChannel);
			}
			else
			{
				outputs[OUTPUT_GATE].setVoltage(0.f, phaseChannel);
			}
		}
	}

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "gateMode", json_integer(gateMode.gateMode));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override
	{
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
		{
			gateMode.setGateMode((GateMode::Modes)json_integer_value(gateModeJ));
		};
	};
};

struct PhaseTriggWidget : ModuleWidget
{
	PhaseTriggWidget(PhaseTrigg *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/PhaseTrigg.svg")));

		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, 16)), module, PhaseTrigg::INPUT_CV));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(3 * HP, 16)), module, PhaseTrigg::INPUT_GATE_PATTERN));

		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 8; j++)
			{
				addParam(createLightParamCentered<VCVLightBezelLatch<RedGreenBlueLight>>(mm2px(Vec(HP + (i * 2 * HP), JACKYSTART + (j)*JACKYSPACE)), module, PhaseTrigg::PARAM_GATE + (j + i * 8), PhaseTrigg::LIGHTS_GATE + (j + i * 8) * 3));
				addChild(createLightCentered<MediumSimpleLight<RedLight>>(mm2px(Vec(HP + (i * 2 * HP) + 0, JACKYSTART + (j)*JACKYSPACE + 0)), module, PhaseTrigg::LIGHTS_ACTIVE_GATE + (j + i * 8)));
			}

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, PhaseTrigg::PARAM_DURATION));
		addChild(createOutputCentered<SmallPort>(mm2px(Vec(3 * HP, LOW_ROW + JACKYSPACE)), module, PhaseTrigg::OUTPUT_GATE));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module, PhaseTrigg::INPUT_DURATION_CV));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_RIGHT_CONNECTED));
	}

	void appendContextMenu(Menu *menu) override
	{
		PhaseTrigg *module = dynamic_cast<PhaseTrigg *>(this->module);
		assert(module);
		menu->addChild(module->gateMode.createMenuItem());

		std::vector<std::string> monitorLabels;

		for (int i = 1; i <= 16; i++)
			monitorLabels.push_back(std::to_string(i));
		menu->addChild(createIndexSubmenuItem(
			"Monitor channel", monitorLabels,
			[=]()
			{ return module->monitorChannel; },
			[=](int i)
			{ module->monitorChannel = i; }));
	}
};

Model *modelPhaseTrigg = createModel<PhaseTrigg, PhaseTriggWidget>("Spike");