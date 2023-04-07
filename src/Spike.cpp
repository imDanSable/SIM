#include "plugin.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "ModuleX.hpp"
#include "GateMode.hpp"
#include "ReX.hpp"
#include "OutX.hpp"
#include "InX.hpp"
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
struct Spike : public ModuleX
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
	int start[NUM_CHANNELS] = {};
	int length[NUM_CHANNELS] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
	// int patternLength[NUM_CHANNELS] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
	int prevChannelIndex[NUM_CHANNELS] = {};
	int prevEditChannel = 0;
	int singleMemory = 0; // 0 16 memory banks, 1 1 memory bank

private:
	array<array<bool, 16>, 16> gateMemory = {};

	dsp::Timer uiTimer;
	float prevCv[NUM_CHANNELS] = {};
	dsp::PulseGenerator triggers[NUM_CHANNELS];

	/* Expander stuff */

	/// @return true if direction is forward
	inline bool getDirection(float cv, float prevCv) const
	{
		const float diff = cv - prevCv;
		if (((diff >= 0) && (diff <= 5.f)) || ((-5.f >= diff) && (diff <= 0)))
			return true;
		else
			return false;
	};

	bool getGate(const uint8_t channelIndex, const uint8_t gateIndex) const
	{
		if (singleMemory == 0)
			return gateMemory[channelIndex][gateIndex];
		else
			return gateMemory[0][gateIndex];
	};

	void setGate(const uint8_t channelIndex, const uint8_t gateIndex, const bool value)
	{
		if (singleMemory == 0)
			gateMemory[channelIndex][gateIndex] = value;
		else
			gateMemory[0][gateIndex] = value;
	};

public:
	Spike() : gateMode(this, PARAM_DURATION)
	{
		this
			->addAllowedModel(modelReX, LEFT)
			->addAllowedModel(modelInX, LEFT)
			->addAllowedModel(modelOutX, RIGHT)
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
		configParam(PARAM_EDIT_CHANNEL, 0.f, 15.f, 0.f, "Edit Channel", "", 0, 1.f, 1.f);
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
	// XXX XXX I believe we can remove num_lights since we are passing inx
	void updateUi(InX *inx, const int num_lights)
	{
		const int channel = params[PARAM_EDIT_CHANNEL].getValue();
		if (prevEditChannel != params[PARAM_EDIT_CHANNEL].getValue())
		{
			for (int i = 0; i < 16; i++)
			{
				params[PARAM_GATE + i].setValue(getGate(channel, i) ? 10.f : 0.f);
			}
			prevEditChannel = params[PARAM_EDIT_CHANNEL].getValue();
		}
		else
		{
			for (int i = 0; i < 16; i++)
			{
				setGate(channel, i, params[PARAM_GATE + i].getValue() > 0.f);
			}
		}

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
				if ((inx) && (inx->inputs[channel].isConnected()))
					light_on = inx->inputs[channel].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f;
				else
					light_on = getGate(channel, buttonIdx % num_lights);
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
				if ((inx) && (inx->inputs[channel].isConnected()))
					light_on = inx->inputs[channel].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f;
				else
					light_on = getGate(channel, buttonIdx % num_lights);
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
				if ((inx) && (inx->inputs[channel].isConnected()))
					light_on = inx->inputs[channel].getNormalPolyVoltage(0, (buttonIdx % num_lights)) > 0.f;
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
		// XXX TODO move this to ModuleX 's onExpanderChange
		this->updateTraversalCache(LEFT);
		this->updateTraversalCache(RIGHT);
		ReX *rex = dynamic_cast<ReX *>(getModel(modelReX, LEFT));
		OutX *outx = dynamic_cast<OutX *>(getModel(modelOutX, RIGHT));
		InX *inx = dynamic_cast<InX *>(getModel(modelInX, LEFT));
		const bool rex_start_cv_connected = rex && rex->inputs[ReX::INPUT_START].isConnected();
		const bool rex_length_cv_connected = rex && rex->inputs[ReX::INPUT_LENGTH].isConnected();
		const int phaseChannelCount = inputs[INPUT_CV].getChannels();
		outputs[OUTPUT_GATE].setChannels(phaseChannelCount);
		const bool ui_update = uiTimer.process(args.sampleTime) > UI_UPDATE_TIME;
		int maxLength = 16;

		// Start runs from 0 to 15 (index)
		// int start = 0;
		// Length runs from 1 to 16 (count) and never zero
		// int length = 16;

		auto calc_start_and_length = [&rex, &inx, /*&patternLength,*/ &rex_start_cv_connected, &rex_length_cv_connected, &maxLength](int channel, int *start, int *length)
		{
			if (rex)
			{
				const float rex_start_cv_input = rex->inputs[ReX::INPUT_START].getNormalPolyVoltage(0, channel);
				const float rex_length_cv_input = rex->inputs[ReX::INPUT_LENGTH].getNormalPolyVoltage(1, channel);
				const int rex_param_start = rex->params[ReX::PARAM_START].getValue();
				const int rex_param_length = rex->params[ReX::PARAM_LENGTH].getValue();

				if (!inx)
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
				else // inx connected
				{
					const int patternLength = inx->inputs[INPUT_GATE_PATTERN].getChannels();

					// XXX Not tested
					const int adjusted_maxLength = patternLength > 0 ? patternLength : maxLength;
					*start = rex_start_cv_connected ?
													// Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								 clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0, maxLength)), 0, maxLength - 1)
													:
													// Clamping prevents out of range values due smaller than 16 range due to pattern length
								 clamp(rex_param_start, 0, adjusted_maxLength - 1);
					*length = rex_length_cv_connected ?
													  // Clamping prevents out of range values due to CV smaller than 0 and bigger than 10
								  clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1, maxLength + 1)), 1, maxLength)
													  :
													  // Clamping prevents out of range values due smaller than 16 range due to pattern length
								  clamp(rex_param_length, 0, adjusted_maxLength);
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
			if (phaseChannelCount == 0) // no input connected. Update ui
			{
				const int editchannel = getParam(PARAM_EDIT_CHANNEL).getValue();
				calc_start_and_length(editchannel, &start[editchannel], &length[editchannel]);
				updateUi(inx, maxLength);
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
				updateUi(inx, maxLength);

			const bool channelChanged = (prevChannelIndex[phaseChannel] != channel_index) && change;
			if (channelChanged) // change bool to assure we don't trigger if ReX is modifying us
			{
				// XXX MAYBE we should check if anything between prevChannelIndex +/- 1
				// and channel_index is set so we can trigger those gates too
				// when the input signal jumps (or have it as an option)

				if (
					(inx && (inx->inputs[phaseChannel].getChannels() > 0) &&
					 inx->inputs[phaseChannel].getNormalPolyVoltage(0, channel_index) > 0) ||
					((!inx /*patternLength == 0*/) &&
					 (getGate(phaseChannel, channel_index) /*params[PARAM_GATE + channel_index].getValue() > 0.f*/)))
				{
					const float adjusted_duration = params[PARAM_DURATION].getValue() * 0.1f * inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.f, channel_index);
					gateMode.triggerGate(phaseChannel, adjusted_duration, phase, length[phaseChannel], direction);
				}
				prevChannelIndex[phaseChannel] = channel_index;
			}
			if (outx)
				outx->outputs[channel_index].setChannels(phaseChannelCount);
			if (gateMode.process(phaseChannel, phase, args.sampleTime))
			{
				bool snooped = false;
				if (outx)
				{
					snooped = outx->setOutput(channel_index, 10.f, phaseChannel, true);
				}
				if (!snooped)
					outputs[OUTPUT_GATE].setVoltage(10.f, phaseChannel);
			}
			else
			{
				outputs[OUTPUT_GATE].setVoltage(0.f, phaseChannel);
				if (outx)
				{
					outx->setOutput(channel_index, 0.f, phaseChannel, true);
				}
			}
		}
	};

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "gateMode", json_integer(gateMode.gateMode));
		json_object_set_new(rootJ, "singleMemory", json_integer(singleMemory));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override
	{
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
		{
			gateMode.setGateMode((GateMode::Modes)json_integer_value(gateModeJ));
		};
		json_t *singleMemoryJ = json_object_get(rootJ, "singleMemory");
		if (singleMemoryJ)
		{
			singleMemory = json_integer_value(singleMemoryJ);
		};
	};
};

struct SpikeWidget : ModuleWidget
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
	SpikeWidget(Spike *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

		addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::INPUT_CV));
		// addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::INPUT_GATE_PATTERN));
		addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::OUTPUT_GATE));
		addChild(createSegment2x8Widget(module, mm2px(Vec(0.f, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART))));

		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 8; j++)
			{
				addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(HP + (i * 2 * HP), JACKYSTART + (j)*JACKYSPACE)), module, Spike::PARAM_GATE + (j + i * 8), Spike::LIGHTS_GATE + (j + i * 8)));
			}

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, Spike::PARAM_DURATION));
		addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module, Spike::INPUT_DURATION_CV));

		addParam(createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, LOW_ROW)), module, Spike::PARAM_EDIT_CHANNEL));

		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, Spike::LIGHT_LEFT_CONNECTED));
		addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, Spike::LIGHT_RIGHT_CONNECTED));
	}
	struct Segment2x8 : widget::Widget
	{
		Spike *module;
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
				nvgCircle(ctx, startVec.x, startVec.y, 12.f);
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
				{
					// Draw for the browser and screenshot
					drawLineSegments(args.vg, 3, 11, 16);
					const float activeGateX = HP;
					const float activeGateY = 6 * JACKYSPACE; // XXX Opt %
					// Active step
					nvgBeginPath(args.vg);
					// const float activeGateRadius = 2.f;
					// const float activeGateWidth = 10.f;
					// nvgRoundedRect(args.vg, mm2px(activeGateX) - activeGateWidth, mm2px(activeGateY) - activeGateWidth, 2 * activeGateWidth, 2 * activeGateWidth, activeGateRadius);
					nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.f);
					nvgFillColor(args.vg, rack::color::WHITE);
					nvgFill(args.vg);
					return;
				}
				const int editChannel = module->params[Spike::PARAM_EDIT_CHANNEL].getValue();
				const int start = module->start[editChannel];
				const int length = module->length[editChannel];
				const int prevChannel = module->prevChannelIndex[editChannel];

				const int maximum = module->inputs[Spike::INPUT_GATE_PATTERN].getChannels() > 0 ? module->inputs[Spike::INPUT_GATE_PATTERN].getChannels() : 16;
				drawLineSegments(args.vg, start, length, maximum);

				const int activeGateCol = prevChannel / 8;
				const float activeGateX = HP + activeGateCol * 2 * HP;
				const float activeGateY = (prevChannel % 8) * JACKYSPACE; // XXX Opt %
				// Active step
				nvgBeginPath(args.vg);
				// const float activeGateRadius = 2.f;
				// const float activeGateWidth = 10.f;
				// nvgRoundedRect(args.vg, mm2px(activeGateX) - activeGateWidth, mm2px(activeGateY) - activeGateWidth, 2 * activeGateWidth, 2 * activeGateWidth, activeGateRadius);
				nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.f);
				nvgFillColor(args.vg, rack::color::WHITE);
				nvgFill(args.vg);
			}
		}
	};
	void appendContextMenu(Menu *menu) override
	{
		Spike *module = dynamic_cast<Spike *>(this->module);
		// if (!module)
		// 	return;
		assert(module);

		{ // Add expander // Thank you Coriander Pines!
			// XXX TODO Autopopulate with compatible modules left and right
			auto item1 = new ModuleInstantionMenuItem;
			item1->text = "Add InX (left, 4HP)";
			item1->module_widget = this;
			item1->right = false;
			item1->hp = 4;
			item1->model = modelInX;
			menu->addChild(item1);

			auto item2 = new ModuleInstantionMenuItem;
			item2->text = "Add ReX (left, 2HP)";
			item2->module_widget = this;
			item2->right = false;
			item2->hp = 2;
			item2->model = modelReX;
			menu->addChild(item2);

			// menu->addChild(createSubmenuItem("Add Expander", "",
			// 								 [=](Menu *menu)
			// 								 {
			// 									 menu->addChild(item);
			// 								 }));
		}

		menu->addChild(module->gateMode.createMenuItem());
		std::vector<std::string> operation_modes = {"One memory bank per Φ input", "Single shared memory bank"};

		menu->addChild(createIndexSubmenuItem(
			"Operation mode",
			operation_modes,
			[=]()
			{ return module->singleMemory; },
			[=](int index)
			{ module->singleMemory = index; }));
	}
	Segment2x8 *createSegment2x8Widget(Spike *module, Vec pos, Vec size)
	{
		Segment2x8 *display = createWidget<Segment2x8>(pos);
		display->module = module;
		display->box.size = size;
		return display;
	};
};

Model *modelSpike = createModel<Spike, SpikeWidget>("Spike");