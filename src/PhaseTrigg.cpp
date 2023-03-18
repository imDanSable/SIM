#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"
#include "GateMode.hpp"
#include "ReXpander.hpp"

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

	inline void setLedColor(int index, float r, float g, float b, bool accent = false)
	{

		lights[0 + index * 3].setBrightness(clamp((0.6 + accent * 0.4) * r));
		lights[1 + index * 3].setBrightness(clamp((0.6 + accent * 0.4) * g));
		lights[2 + index * 3].setBrightness(clamp((0.6 + accent * 0.4) * b));
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
	void updateUi(const int start, const int length)
	{

		for (int i = 0; i < 16; i++)
		{
			lights[LIGHTS_ACTIVE_GATE + i].setBrightness(0.f);
		}

		for (int i = 0; i < start; i++)
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				params[(PARAM_GATE + i % 16)].setValue(inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % 16)) > 0.f); // % not tested in_start/in_length
			bool gate = params[(PARAM_GATE + i) % 16].getValue() > 0.f;

			setLedColor((LIGHTS_GATE + i) % 16, gate * 0.15f, gate * 0.15f, gate * 0.15f);
		}
		setLedColor((LIGHTS_GATE + start) % 16, 0.f, 1.f, 0.f, params[(PARAM_GATE + start) % 16].getValue() > 0.f);
		for (int i = start + 1; i < start + length - 1; i++)
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				params[(PARAM_GATE + i % 16)].setValue(inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % 16)) > 0.f); // % not tested in_start/in_length
			bool gate = params[(PARAM_GATE + i) % 16].getValue() > 0.f;

			setLedColor((LIGHTS_GATE + i) % 16, gate * 1.f, gate * 1.f, gate * 1.f);
		}
		setLedColor((LIGHTS_GATE + start + length - 1) % 16, 1.f, 0.f, 0.f, params[(PARAM_GATE + start + length - 1) % 16].getValue() > 0.f);
		for (int i = start + length; i < 16; i++)
		{
			if (inputs[INPUT_GATE_PATTERN].isConnected())
				params[(PARAM_GATE + i % 16)].setValue(inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (i % 16)) > 0.f); // % not tested in_start/in_length
			bool gate = params[(PARAM_GATE + i) % 16].getValue() > 0.f;

			setLedColor((LIGHTS_GATE + i) % 16, gate * 0.15f, gate * 0.15f, gate * 0.15f);
		}
	}

	void process(const ProcessArgs &args) override
	{
		ModuleX::process(args);
		ReXpander *rex;
		rex = dynamic_cast<ReXpander *>(getModel(modelReXpander, LEFT));
		const int phaseChannelCount = inputs[INPUT_CV].getChannels();
		outputs[OUTPUT_GATE].setChannels(phaseChannelCount);
		bool ui_update = false;
		if (uiTimer.process(args.sampleTime) > UI_UPDATE_TIME)
		{
			uiTimer.reset();
			ui_update = true;
			if (phaseChannelCount == 0)
			{
				if (rex)
					updateUi(rex->params[ReXpander::PARAM_START].getValue(), rex->params[ReXpander::PARAM_LENGTH].getValue());
				else
					updateUi(0, 16);
			}
		}

		for (int phaseChannel = 0; phaseChannel < phaseChannelCount; phaseChannel++)
		{

			int start = 0;
			int length = 16;
			const bool patternConnected = inputs[INPUT_GATE_PATTERN].isConnected();
			if (patternConnected)
			{
				length = inputs[INPUT_GATE_PATTERN].getChannels();
			}
			if (rex)
			{
				if (rex->inputs[ReXpander::INPUT_START].isConnected())
				{
					if (patternConnected) // ReX+ReXinput + Pattern
					{
						start = clamp(rescale(rex->inputs[ReXpander::INPUT_START].getNormalPolyVoltage(0, phaseChannel), 0.f, 10.f, 0.f, length - 1), 0.f, length - 1.f);
					}
					else
					{ // ReX+ReXinput
						start = clamp(rescale(rex->inputs[ReXpander::INPUT_START].getNormalPolyVoltage(0, phaseChannel), 0.f, 10.f, 0.f, 15.f), 0.f, 15.f);
					}
				}
				else
				{
					if (patternConnected) // ReX+Pattern
					{
						start = clamp(rex->params[ReXpander::PARAM_START].getValue(), 0.f, length - 1.f);
					}
					else
					{ // Rex
						start = rex->params[ReXpander::PARAM_START].getValue();
					}
				}

				if (rex->inputs[ReXpander::INPUT_LENGTH].isConnected())
				{
					if (patternConnected) // ReX+ReXinput + Pattern
					{
						length = clamp(rescale(rex->inputs[ReXpander::INPUT_LENGTH].getNormalPolyVoltage(0, phaseChannel), 0.f, 10.f, 1.f, float(length - start)), 1.f, float(length - start));
					}
					else
					{ // ReX+ReXinput
						length = clamp(rescale(rex->inputs[ReXpander::INPUT_LENGTH].getNormalPolyVoltage(0, phaseChannel), 0.f, 10.f, 1.f, 16.f), 1.f, 16.f);
					}
				}
				else
				{
					if (patternConnected)
					{
						length = clamp(rex->params[ReXpander::PARAM_LENGTH].getValue(), 1.f, float(length - start));
					}
					else
					{
						length = rex->params[ReXpander::PARAM_LENGTH].getValue();
					}
				}
				// length = rex->params[ReXpander::PARAM_LENGTH].getValue();
			}

			const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, phaseChannel);
			const bool change = (cv != prevCv[phaseChannel]);
			const float phase = cv / 10.f; // 10.0001 To prevent rounding up at edges
			const bool direction = getDirection(cv, prevCv[phaseChannel]);
			prevCv[phaseChannel] = cv;
			const int channel_index = int(((clamp(int(floor(length * (phase))), 0, length - 1)) + start)) % 16;

			if ((phaseChannel == 0) && ui_update)
			{
				updateUi(start, length);
			}

			if (ui_update)
				lights[LIGHTS_ACTIVE_GATE + channel_index].setBrightness(1.f);

			if ((prevChannelIndex[phaseChannel] != (channel_index)) && change) // change bool to assure we don't trigger if ReXpander is modifying us
			{
				// XXX MAYBE we should see if anything between prevChannelIndex +/- 1
				// and channel_index is set so we can trigger those gates too
				// when the input signal jumps

				if (params[PARAM_GATE + channel_index].getValue() > 0.f)
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
				addChild(createLightCentered<TinySimpleLight<BlueLight>>(mm2px(Vec(HP + (i * 2 * HP) + 0, JACKYSTART + (j)*JACKYSPACE + 0)), module, PhaseTrigg::LIGHTS_ACTIVE_GATE + (j + i * 8)));
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
	}
};

Model *modelPhaseTrigg = createModel<PhaseTrigg, PhaseTriggWidget>("Spike");