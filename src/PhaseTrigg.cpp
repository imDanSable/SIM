#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"
#include "GateMode.hpp"
#include "ReXpander.hpp"
#include <array>
#include <bitset>
#include <utility>
#include <cmath>

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
		PARAM_EDIT_CHANNEL,
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
		ENUMS(LIGHTS_GATE, 16),
		LIGHT_LEFT_CONNECTED,
		LIGHT_RIGHT_CONNECTED,
		LIGHTS_LEN
	};

	GateMode gateMode;
	// XXX TODO Make private and friend widget
	int start[NUM_CHANNELS] = {};
	int length[NUM_CHANNELS] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
	int prevChannelIndex[NUM_CHANNELS] = {};
	// int prevEditChannel = 0;

private:
	array<bitset<16>, 16> gateMemory = {};

	dsp::Timer uiTimer;
	float prevCv[NUM_CHANNELS] = {};
	dsp::PulseGenerator triggers[NUM_CHANNELS];

	/* Expander stuff */

	//@return true if direction is forward
	inline bool getDirection(float cv, float prevCv) const
	{
		const float diff = cv - prevCv;
		if (((diff >= 0) && (diff <= 5.f)) || ((-5.f >= diff) && (diff <= 0)))
			return true;
		else
			return false;
	};

	bool getGate(const uint8_t channelIndex, const uint8_t gateIndex) //const
	{
		return params[PARAM_GATE + gateIndex].getValue() > 0.0f;
		// return gateMemory[channelIndex][gateIndex];
	};

	void setGate(const uint8_t channelIndex, const uint8_t gateIndex, const bool value)
	{
		// gateMemory[channelIndex][gateIndex] = value;
		params[PARAM_GATE + gateIndex].setValue(value ? 10.0f : 0.0f);

	};

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
		configParam(PARAM_EDIT_CHANNEL, 0.f, 15.f, 1.f, "Edit Channel", "", 0, 1.f, 1.f);
		getParamQuantity(PARAM_EDIT_CHANNEL)->snapEnabled = true;

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
	void updateUi(const int channel, const int num_lights)
	{
		const int start = this->start[channel];
		const int length = this->length[channel];

		for (int i = 0; i < 16; i++) // reset all lights
		{
			// black
			lights[LIGHTS_GATE + i].setBrightness(0.f);
		}

		bool light_on;

		// Special case length == num_lights
		if (length == num_lights)
		{
			for (int buttonIdx = 0; buttonIdx < num_lights; ++buttonIdx)
			{
				if (inputs[INPUT_GATE_PATTERN].isConnected())
					light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f; // % not tested in_start/in_length
				else
					light_on = getGate(channel, buttonIdx % num_lights);
					// params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
				if (light_on)
					// gray
					lights[LIGHTS_GATE + buttonIdx].setBrightness(1.f);
			}
		}
		else
		{
			// Out of active range
			int end = (start + length - 1) % num_lights;
			// Start beyond the end of the inner range
			int buttonIdx = (end + 1) % num_lights;
			while (buttonIdx != start)
			{
				if (inputs[INPUT_GATE_PATTERN].isConnected())
					light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f; // % not tested in_start/in_length
				else
					light_on = getGate(channel, buttonIdx % num_lights);
					// params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
				if (light_on)
					// gray
					lights[LIGHTS_GATE + buttonIdx].setBrightness(0.2f);

				// Increment the button index for the next iteration, wrapping around with modulo
				buttonIdx = (buttonIdx + 1) % num_lights;
			}

			// Active lights
			buttonIdx = start;
			while (buttonIdx != ((end + 1) % num_lights))
			{
				if (inputs[INPUT_GATE_PATTERN].isConnected())
					light_on = inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f; // % not tested in_start/in_length
				else
					light_on = getGate(channel, buttonIdx % num_lights);
					// params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
				if (light_on)
					// white
					lights[LIGHTS_GATE + buttonIdx].setBrightness(1.f);

				// Increment the button index for the next iteration, wrapping around with modulo
				buttonIdx = (buttonIdx + 1) % num_lights;
			}
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

		const int maxLength = patternLength > 0 ? patternLength : 16;

		// Start runs from 0 to 15 (index)
		// int start = 0;
		// Length runs from 1 to 16 (count) and never zero
		// int length = 16;

		auto calc_start_and_length = [&rex, &patternLength, &rex_start_cv_connected, &rex_length_cv_connected, &maxLength](int channel, int *start, int *length)
		{
			if (rex)
			{
				const float rex_start_cv_input = rex->inputs[ReXpander::INPUT_START].getNormalPolyVoltage(0, channel);
				const float rex_length_cv_input = rex->inputs[ReXpander::INPUT_LENGTH].getNormalPolyVoltage(1, channel);
				const int rex_param_start = rex->params[ReXpander::PARAM_START].getValue();
				const int rex_param_length = rex->params[ReXpander::PARAM_LENGTH].getValue();
				if (patternLength == 0) // duration not connected
				{
					*start = rex_start_cv_connected ?
												   // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0, maxLength)), 0, maxLength - 1)
												   : rex_param_start;
					*length = rex_length_cv_connected ?
													 // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								 clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1, maxLength + 1)), 1, maxLength)
													 : rex_param_length;
				}
				else // duration connected
				{
					*start = rex_start_cv_connected ?
												   // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0, maxLength)), 0, maxLength - 1)
												   :
												   // Clamping prevents out of range values due smaller than 16 range due to pattern length
								clamp(rex_param_start, 0, maxLength - 1);
					*length = rex_length_cv_connected ?
													 // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								 clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1, maxLength + 1)), 1, maxLength)
													 :
													 // Clamping prevents out of range values due smaller than 16 range due to pattern length
								 clamp(rex_param_length, 0, maxLength);
				}
			}
			else
			{
				*start = 0;
				*length = maxLength;
			}
		};

		if (ui_update)
		{
			uiTimer.reset();
			if (phaseChannelCount == 0) // no input connected. Update
			{
				const int editchannel = getParam(PARAM_EDIT_CHANNEL).getValue();
				calc_start_and_length(editchannel, &start[editchannel], &length[editchannel]);
				updateUi(editchannel, maxLength);
			}
		}
		for (int phaseChannel = 0; phaseChannel < phaseChannelCount; phaseChannel++)
		{

			calc_start_and_length(phaseChannel, &start[phaseChannel], &length[phaseChannel]);

			const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, phaseChannel);
			const bool change = (cv != prevCv[phaseChannel]);
			const float phase = clamp(cv / 10.f, 0.00001f, .99999); // to avoid jumps at 0 and 1
			const bool direction = getDirection(cv, prevCv[phaseChannel]);
			prevCv[phaseChannel] = cv;
			const int channel_index = int(((clamp(int(floor(length[phaseChannel] * (phase))), 0, length[phaseChannel])) + start[phaseChannel])) % int(maxLength);

			if ((phaseChannel == params[PARAM_EDIT_CHANNEL].getValue()) && ui_update)
				updateUi(phaseChannel, maxLength);

			if ((prevChannelIndex[phaseChannel] != (channel_index)) && change) // change bool to assure we don't trigger if ReXpander is modifying us
			{
				// XXX MAYBE we should see if anything between prevChannelIndex +/- 1
				// and channel_index is set so we can trigger those gates too
				// when the input signal jumps

				if (((inputs[INPUT_GATE_PATTERN].getChannels() > 0) && inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, channel_index) > 0) || ((patternLength == 0) && (getGate(phaseChannel, channel_index) /*params[PARAM_GATE + channel_index].getValue() > 0.f*/)))
				{
					const float adjusted_duration = params[PARAM_DURATION].getValue() * 0.1f * inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.f, channel_index);
					gateMode.triggerGate(phaseChannel, adjusted_duration, phase, length[phaseChannel], direction);
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

	template <typename TLight>
	struct SIMLightLatch : VCVLightLatch<TLight>
	{
		SIMLightLatch()
		{
			this->momentary = false;
			this->latch = true;
			this->frames.clear();
			this->addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/SIMLightButton_0.svg")));
			this->addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/SIMLightButton_1.svg")));
			this->sw->setSvg(this->frames[0]);
			this->fb->dirty = true;
			math::Vec svgSize = this->sw->box.size;
			this->box.size = svgSize;
			this->shadow->box.pos = math::Vec(0, 1.1 * svgSize.y);
			this->shadow->box.size = svgSize;
		}
		void step() override
		{
			VCVLightLatch<TLight>::step();
			math::Vec center = this->box.size.div(2);
			this->light->box.pos = center.minus(this->light->box.size.div(2));
			if (this->shadow)
			{
				// Update the shadow position to match the center of the button
				this->shadow->box.pos = center.minus(this->shadow->box.size.div(2).plus(math::Vec(0, -1.5f)));
			}
		}
	};
	PhaseTriggWidget(PhaseTrigg *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/PhaseTrigg.svg")));

		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, 16)), module, PhaseTrigg::INPUT_CV));
		// addInput(createInputCentered<SmallPort>(mm2px(Vec(3 * HP, 16)), module, PhaseTrigg::INPUT_GATE_PATTERN));
		addChild(createOutputCentered<SmallPort>(mm2px(Vec(3 * HP, 16)), module, PhaseTrigg::OUTPUT_GATE));
		addChild(createSegment2x8Widget(module, mm2px(Vec(0.f, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART))));

		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 8; j++)
			{
				addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(HP + (i * 2 * HP), JACKYSTART + (j)*JACKYSPACE)), module, PhaseTrigg::PARAM_GATE + (j + i * 8), PhaseTrigg::LIGHTS_GATE + (j + i * 8)));
			}

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, PhaseTrigg::PARAM_DURATION));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module, PhaseTrigg::INPUT_DURATION_CV));

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, LOW_ROW)), module, PhaseTrigg::PARAM_EDIT_CHANNEL));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_RIGHT_CONNECTED));
	}
	struct Segment2x8 : widget::Widget
	{
		PhaseTrigg *module;
		void draw(const DrawArgs &args) override
		{
			drawLayer(args, 0);
		}

		void drawLine(NVGcontext *ctx, int startCol, int startInCol, int endInCol, bool actualStart, bool actualEnd)
		{
			Vec startVec = mm2px(Vec(HP + startCol * 2 * HP, startInCol * JACKYSPACE));
			Vec endVec = mm2px(Vec(HP + startCol * 2 * HP, endInCol * JACKYSPACE));

			if (startInCol == endInCol)
			{
				nvgFillColor(ctx, panelBlue);
				nvgBeginPath(ctx);
				nvgCircle(ctx, startVec.x, startVec.y, 10.f);
				nvgFill(ctx);
			}
			else
			{
				nvgStrokeColor(ctx, panelPink);
				nvgLineCap(ctx, NVG_ROUND);
				nvgStrokeWidth(ctx, 20.f);

				nvgBeginPath(ctx);
				nvgMoveTo(ctx, startVec.x, startVec.y);
				nvgLineTo(ctx, endVec.x, endVec.y);
				nvgStroke(ctx);
				if (actualStart)
				{
					nvgFillColor(ctx, panelBlue);
					nvgBeginPath(ctx);
					nvgCircle(ctx, startVec.x, startVec.y, 10.f);
					nvgRect(ctx, startVec.x - 10.f, startVec.y, 20.f, 10.f);
					nvgFill(ctx);
				}
				if (actualEnd)
				{
					nvgFillColor(ctx, panelBlue);
					nvgBeginPath(ctx);
					nvgCircle(ctx, endVec.x, endVec.y, 10.f);
					nvgRect(ctx, endVec.x - 10.f, endVec.y - 10.f, 20.f, 10.f);
					nvgFill(ctx);
				}
			}
		}

		void drawLineSegments(NVGcontext *ctx, int start, int length, int maxLength)
		{
			int columnSize = 8;
			int end = (start + length - 1) % maxLength;

			int startCol = start / columnSize;
			int endCol = end / columnSize;
			int startInCol = start % columnSize;
			int endInCol = end % columnSize;

			if (startCol == endCol && start <= end)
			{
				drawLine(ctx, startCol, startInCol, endInCol, true, true);
			}
			else
			{
				if (startCol == 0)
				{
					drawLine(ctx, startCol, startInCol, min(maxLength - 1, columnSize - 1), true, false);
					drawLine(ctx, endCol, 0, endInCol, false, true);
				}
				else
				{
					drawLine(ctx, startCol, startInCol, min((maxLength - 1) % columnSize, columnSize - 1), true, false);
					drawLine(ctx, endCol, 0, endInCol, false, true);
				}

				if (length > columnSize)
				{
					if (startCol == endCol)
					{
						if ((startCol != 0) && (maxLength > columnSize))
						{
							drawLine(ctx, !startCol, 0, columnSize - 1, false, false);
						}
						else
						{
							drawLine(ctx, !startCol, 0, min(columnSize - 1, (maxLength - 1) % columnSize), false, false);
						}
					}
				}
			}
		};

		void drawLayer(const DrawArgs &args, int layer) override
		{
			if (layer == 0)
			{
				if (!module)
					return;
				const int editChannel = module->params[PhaseTrigg::PARAM_EDIT_CHANNEL].getValue();
				const int start = module->start[editChannel];
				const int length = module->length[editChannel];
				const int prevChannel = module->prevChannelIndex[editChannel];

				const int maximum = module->inputs[PhaseTrigg::INPUT_GATE_PATTERN].getChannels() > 0 ? module->inputs[PhaseTrigg::INPUT_GATE_PATTERN].getChannels() : 16;
				WARN("Channel %d, start %d, length %d, max %d", editChannel, start, length, maximum);
				drawLineSegments(args.vg, start, length, maximum);

				const int activeGateCol = prevChannel / 8;
				const float activeGateX = HP + activeGateCol * 2 * HP;
				const float activeGateY = (prevChannel % 8) * JACKYSPACE;
				// Active phase
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 8.f);
				nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
				nvgFill(args.vg);
			}
		}
	};
	void appendContextMenu(Menu *menu) override
	{
		PhaseTrigg *module = dynamic_cast<PhaseTrigg *>(this->module);
		assert(module);
		menu->addChild(module->gateMode.createMenuItem());

		// std::vector<std::string> monitorLabels;

		// for (int i = 1; i <= 16; i++)
		// 	monitorLabels.push_back(std::to_string(i));
		// menu->addChild(createIndexSubmenuItem(
		// 	"Monitor channel", monitorLabels,
		// 	[=]()
		// 	{ return module->editChannel; },
		// 	[=](int i)
		// 	{ module->editChannel = i; }));
	}
	Segment2x8 *createSegment2x8Widget(PhaseTrigg *module, Vec pos, Vec size)
	{
		Segment2x8 *display = createWidget<Segment2x8>(pos);
		display->module = module;
		display->box.size = size;
		return display;
	};
};

Model *modelPhaseTrigg = createModel<PhaseTrigg, PhaseTriggWidget>("Spike");