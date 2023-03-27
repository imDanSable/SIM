#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"
#include "GateMode.hpp"
#include "ReXpander.hpp"
#include <array>
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
	int monitorChannel = 0;
	int start[NUM_CHANNELS] = {};
	int length[NUM_CHANNELS] = {};
	int prevChannelIndex[NUM_CHANNELS] = {};
	struct RangeData
	{
		bool valid = false;
		int8_t start = 0;
		int8_t end = 0;
		bool col = 0;
	};
	RangeData lineSegments[3];

private:
	dsp::Timer uiTimer;
	float prevCv[NUM_CHANNELS] = {};
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
		getRangeData(start, length, num_lights, lineSegments);

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
					light_on = params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
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
					light_on = params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
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
					light_on = params[(PARAM_GATE + buttonIdx) % num_lights].getValue() > 0.f;
				if (light_on)
					// white
					lights[LIGHTS_GATE + buttonIdx].setBrightness(1.f);

				// Increment the button index for the next iteration, wrapping around with modulo
				buttonIdx = (buttonIdx + 1) % num_lights;
			}
		}
	}

	void getRangeData(const int start, const int length, const int maximum, RangeData *rangeData)
	{
		int end = (start + length - 1) % maximum;
		int columnSize = 8;

		int startCol = start / columnSize;
		int endCol = end / columnSize;
		int startInCol = start % columnSize;
		int endInCol = end % columnSize;

		if (startCol == endCol && start <= end)
		{
			rangeData[0].valid = true;
			rangeData[0].start = startInCol;
			rangeData[0].end = endInCol;
			rangeData[0].col = startCol;
		}
		else
		{
			int endFirstColumn = columnSize - 1;
			int startInCol = start % columnSize;
			int endInCol = end % columnSize;

			rangeData[0].valid = true;
			rangeData[0].start = startInCol;
			rangeData[0].end = endFirstColumn;
			rangeData[0].col = startCol;

			rangeData[1].valid = true;
			rangeData[1].start = 0;
			rangeData[1].end = endInCol;
			rangeData[1].col = endCol;

			if (length > columnSize)
			{
				if (startCol == endCol)
				{
					rangeData[2].valid = true;
					rangeData[2].start = 0;
					rangeData[2].end = 7;
					rangeData[2].col = !startCol;
				}
			}
		}
	};

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

		auto calc_start_and_length = [&rex, &patternLength, &rex_start_cv_connected, &rex_length_cv_connected, &maxLength](int channel, int &start, int &length)
		{
			if (rex)
			{
				const float rex_start_cv_input = rex->inputs[ReXpander::INPUT_START].getNormalPolyVoltage(0, channel);
				const float rex_length_cv_input = rex->inputs[ReXpander::INPUT_LENGTH].getNormalPolyVoltage(0, channel);
				const int rex_param_start = rex->params[ReXpander::PARAM_START].getValue();
				const int rex_param_length = rex->params[ReXpander::PARAM_LENGTH].getValue();
				if (patternLength == 0)
				{
					start = rex_start_cv_connected ?
												   // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0, maxLength)), 0, maxLength - 1)
												   : rex_param_start;
					length = rex_length_cv_connected ?
													 // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								 clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1, maxLength + 1)), 1, maxLength)
													 : rex_param_length;
				}
				else
				{
					start = rex_start_cv_connected ?
												   // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0, maxLength)), 0, maxLength - 1)
												   :
												   // Clamping prevents out of range values due smaller than 16 range due to pattern length
								clamp(rex_param_start, 0, maxLength - 1);
					length = rex_length_cv_connected ?
													 // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								 clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1, maxLength + 1)), 1, maxLength)
													 :
													 // Clamping prevents out of range values due smaller than 16 range due to pattern length
								 clamp(rex_param_length, 0, maxLength);
				}
			}
			else
			{
				start = 0;
				length = maxLength;
			}
		};

		if (ui_update)
		{
			uiTimer.reset();
			if (phaseChannelCount == 0) // no input connected. Update
			{
				calc_start_and_length(0, start[0], length[0]);
				updateUi(start[0], length[0], maxLength);
			}
		}
		for (int phaseChannel = 0; phaseChannel < phaseChannelCount; phaseChannel++)
		{

			calc_start_and_length(phaseChannel, start[phaseChannel], length[phaseChannel]);

			const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, phaseChannel);
			const bool change = (cv != prevCv[phaseChannel]);
			const float phase = clamp(cv / 10.f, 0.00001f, .99999); // to avoid jumps at 0 and 1
			const bool direction = getDirection(cv, prevCv[phaseChannel]);
			prevCv[phaseChannel] = cv;
			const int channel_index = int(((clamp(int(floor(length[phaseChannel] * (phase))), 0, length[phaseChannel])) + start[phaseChannel])) % int(maxLength);

			if ((phaseChannel == monitorChannel) && ui_update)
				updateUi(start[phaseChannel], length[phaseChannel], maxLength);

			if ((prevChannelIndex[phaseChannel] != (channel_index)) && change) // change bool to assure we don't trigger if ReXpander is modifying us
			{
				// XXX MAYBE we should see if anything between prevChannelIndex +/- 1
				// and channel_index is set so we can trigger those gates too
				// when the input signal jumps

				if (((inputs[INPUT_GATE_PATTERN].getChannels() > 0) && inputs[INPUT_GATE_PATTERN].getNormalPolyVoltage(0, channel_index) > 0) || ((patternLength == 0) && (params[PARAM_GATE + channel_index].getValue() > 0.f)))
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
	PhaseTriggWidget(PhaseTrigg *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/PhaseTrigg.svg")));

		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, 16)), module, PhaseTrigg::INPUT_CV));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(3 * HP, 16)), module, PhaseTrigg::INPUT_GATE_PATTERN));

		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 8; j++)
			{
				addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(HP + (i * 2 * HP), JACKYSTART + (j)*JACKYSPACE)), module, PhaseTrigg::PARAM_GATE + (j + i * 8), PhaseTrigg::LIGHTS_GATE + (j + i * 8)));
			}

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, PhaseTrigg::PARAM_DURATION));
		addChild(createOutputCentered<SmallPort>(mm2px(Vec(3 * HP, LOW_ROW + JACKYSPACE)), module, PhaseTrigg::OUTPUT_GATE));
		addInput(createInputCentered<SmallPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module, PhaseTrigg::INPUT_DURATION_CV));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, PhaseTrigg::LIGHT_RIGHT_CONNECTED));
	}

	void draw(const DrawArgs &args) override
	{
		drawLayer(args, 1000);
		ModuleWidget::draw(args);
	}

	void drawLayer(const DrawArgs &args, int layer) override
	{
		if (layer == 1000)
		{

			PhaseTrigg *module = dynamic_cast<PhaseTrigg *>(this->module);
			if (!module)
				return;
			const int monitorChannel = module->monitorChannel;
			const int start = module->start[module->monitorChannel];
			const int length = module->length[module->monitorChannel];
			const int prevChannel = module->prevChannelIndex[monitorChannel];
			// Background
			nvgFillColor(args.vg, panelBgColor);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
			nvgFill(args.vg);

			auto drawLineSegment = [args](const PhaseTrigg::RangeData &lineSegment)
			{
				Vec startVec = mm2px(Vec(HP + lineSegment.col * 2 * HP, JACKYSTART + lineSegment.start * JACKYSPACE));
				Vec endVec = mm2px(Vec(HP + lineSegment.col * 2 * HP, JACKYSTART + lineSegment.end * JACKYSPACE));
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, startVec.x, startVec.y);
				nvgLineTo(args.vg, endVec.x, endVec.y + 0.01f); // add 0.01f to draw a 'circle' if startVec1 == endVec1
				nvgStroke(args.vg);
			};

			// Set up the line for inner segments
			nvgStrokeColor(args.vg, panelPink);
			nvgLineCap(args.vg, NVG_ROUND);
			nvgStrokeWidth(args.vg, 20.f);

			const int maximum = module->inputs[PhaseTrigg::INPUT_GATE_PATTERN].getChannels() > 0 ? module->inputs[PhaseTrigg::INPUT_GATE_PATTERN].getChannels() : 16;
			PhaseTrigg::RangeData rangeData[3] = {};
			module->getRangeData(start, length, maximum, rangeData);
			for (const auto &lineSegment : rangeData)
			{
				if (!lineSegment.valid)
					continue;
				else
					drawLineSegment(lineSegment);
			}
			// Draw an arc for the beginning
			nvgBeginPath(args.vg);
			nvgArc(args.vg, mm2px(HP + rangeData[0].col * 2 * HP), mm2px(JACKYSTART + rangeData[0].start * JACKYSPACE), 10.f, 0.f, M_PI, NVG_CCW);
			nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
			nvgFill(args.vg);

			int end = (start + length - 1) % 16;
			int columnSize = 16 / 2;
			int endCol = end / columnSize;
			int endInCol = end % columnSize;
			// Draw an arc for the end
			nvgBeginPath(args.vg);
			nvgArc(args.vg, mm2px(HP + endCol * 2 * HP), mm2px(JACKYSTART + endInCol * JACKYSPACE), 10.f, 0.f, M_PI, NVG_CW);
			nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
			nvgFill(args.vg);

			// if (length > 1)
			// {
			// 	// draw two lines from the arc endings a bit downward
			// 	nvgBeginPath(args.vg);
			// 	nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
			// 	const float downward = 5.f;
			// 	const float half_thickness = .5f;
			// 	nvgMoveTo(args.vg, mm2px(HP + rangeData[0].col * 2 * HP) - 10.f + half_thickness, mm2px(JACKYSTART + rangeData[0].start * JACKYSPACE));
			// 	nvgLineTo(args.vg, mm2px(HP + rangeData[0].col * 2 * HP) - 10.f + half_thickness, mm2px(JACKYSTART + rangeData[0].start * JACKYSPACE) + downward);

			// 	nvgMoveTo(args.vg, mm2px(HP + rangeData[0].col * 2 * HP) + 10.f - half_thickness, mm2px(JACKYSTART + rangeData[0].start * JACKYSPACE));
			// 	nvgLineTo(args.vg, mm2px(HP + rangeData[0].col * 2 * HP) + 10.f - half_thickness, mm2px(JACKYSTART + rangeData[0].start * JACKYSPACE) + downward);
			// 	nvgStrokeWidth(args.vg, 2 * half_thickness);
			// 	nvgStroke(args.vg);
			// 	// // draw two lines from the arc endings a bit downward
			// 	nvgBeginPath(args.vg);
			// 	nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
			// 	nvgMoveTo(args.vg, mm2px(HP + endCol * 2 * HP) - 10.f + half_thickness, mm2px(JACKYSTART + endInCol * JACKYSPACE));
			// 	nvgLineTo(args.vg, mm2px(HP + endCol * 2 * HP) - 10.f + half_thickness, mm2px(JACKYSTART + endInCol * JACKYSPACE) - downward);
			// 	nvgMoveTo(args.vg, mm2px(HP + endCol * 2 * HP) + 10.f - half_thickness, mm2px(JACKYSTART + endInCol * JACKYSPACE));
			// 	nvgLineTo(args.vg, mm2px(HP + endCol * 2 * HP) + 10.f - half_thickness, mm2px(JACKYSTART + endInCol * JACKYSPACE) - downward);
			// 	nvgStrokeWidth(args.vg, 2 * half_thickness);
			// 	nvgStroke(args.vg);
			// }

			const int activeGateCol = prevChannel / 8;
			const float activeGateX = HP + activeGateCol * 2 * HP;
			const float activeGateY = JACKYSTART + (prevChannel % 8) * JACKYSPACE;
			// Active phase
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, mm2px(activeGateX) - 10.f, mm2px(activeGateY) - 10.f, 20.f, 20.f, 5.f);
			nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
			nvgFill(args.vg);
		}
		ModuleWidget::drawLayer(args, layer);
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