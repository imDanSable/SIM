#include "Connectable.hpp"
#include "Expandable.hpp"
#include "GateMode.hpp"
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "helpers.hpp"
#include "plugin.hpp"
#include "ui/MenuSeparator.hpp"
#include <array>
#include <cassert>
#include <cmath>

using constants::LEFT;
using constants::NUM_CHANNELS;
using constants::RIGHT;

// XXX TODO updateui & segment2x8 & triggering need to share code for getting
// the start length max and active
struct Spike : Expandable<Spike>
{
    Spike(const Spike &) = delete;
    Spike(Spike &&) = delete;
    Spike &operator=(const Spike &) = delete;
    Spike &operator=(Spike &&) = delete;

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
        // INPUT_GATE_PATTERN,
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

    enum PolyphonySource
    {
        PHI,
        REX_CV_START,
        REX_CV_LENGTH,
        INX
    };

  private:
    friend struct SpikeWidget;

    ReX *rex = nullptr;
    OutX *outx = nullptr;
    InX *inx = nullptr;

    PolyphonySource polyphonySource = PHI;
    bool connectEnds = false;
    int prevEditChannel = 0;
    int singleMemory = 0; // 0 16 memory banks, 1 1 memory bank
    std::array<int, NUM_CHANNELS> prevChannelIndex = {};
    GateMode gateMode;

    std::array<int, NUM_CHANNELS> start = {};
    std::array<int, NUM_CHANNELS> length = {16, 16, 16, 16, 16, 16, 16, 16,
                                            16, 16, 16, 16, 16, 16, 16, 16};
    std::array<int, NUM_CHANNELS> max = {16, 16, 16, 16, 16, 16, 16, 16,
                                         16, 16, 16, 16, 16, 16, 16, 16};

    std::array<float, NUM_CHANNELS> prevCv = {};
    std::array<std::array<bool, 16>, 16> gateMemory = {};

    dsp::Timer uiTimer;
    std::array<dsp::PulseGenerator, NUM_CHANNELS> triggers;

    /// @return true if direction is forward
    static bool getDirection(float cv, float prevCv)
    {
        const float diff = cv - prevCv;
        return ((diff >= 0) && (diff <= 5.F)) || ((-5.F >= diff) && (diff <= 0));
    };

    /// @return the phase jump since prevCv
    static float getPhaseSpeed(float cv, float prevCv)
    {
        const float diff = cv - prevCv;
        if (diff > 5.F)
        {
            return diff - 10.F;
        }
        if (diff < -5.F)
        {
            return diff + 10.F;
        }
        return diff;
    };

    bool getGate(int channelIndex, int gateIndex) const
    {
        if (singleMemory == 0)
        {
            return gateMemory[channelIndex][gateIndex];
        }
        return gateMemory[0][gateIndex];
    };

    void setGate(int channelIndex, int gateIndex, bool value)
    {
        assert(channelIndex < constants::NUM_CHANNELS); // NOLINT
        assert(gateIndex < constants::NUM_CHANNELS);    // NOLINT
        if (singleMemory == 0)
        {
            gateMemory[channelIndex][gateIndex] = value;
        }
        else
        {
            gateMemory[0][gateIndex] = value;
        }
    };

  public:
    Spike()
        : Expandable(
              {modelReX, modelInX}, {modelOutX},
              [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
              [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); }),
          gateMode(this, PARAM_DURATION)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(INPUT_CV, "Φ-in");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.1F, 100.F, 0.1F, "Gate duration", "%", 0, 1.F);
        configParam(PARAM_EDIT_CHANNEL, 0.F, 15.F, 0.F, "Edit Channel", "", 0, 1.F, 1.F);
        getParamQuantity(PARAM_EDIT_CHANNEL)->snapEnabled = true;

        for (uint_fast8_t i = 0; i < 16; i++)
        {
            configParam(PARAM_GATE + i, 0.0F, 1.0F, 0.0F, "Gate " + std::to_string(i + 1));
        }

        updateLeftExpanders();
        updateRightExpanders();
    }
    ~Spike() override
    {
        if (rex != nullptr)
        {
            rex->setChainChangeCallback(nullptr);
        }
        if (outx != nullptr)
        {
            outx->setChainChangeCallback(nullptr);
        }
        if (inx != nullptr)
        {
            inx->setChainChangeCallback(nullptr);
        }
    }

    void onReset() override
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++)
        {
            prevCv.at(i) = 0.F;
            prevChannelIndex.at(i) = 0;
        }
        prevEditChannel = 0;
    }
    // XXX XXX I believe we can remove num_lights since we are passing inx
    void updateUi(InX *inx, int num_lights)
    {
        const int channel = static_cast<int>(params[PARAM_EDIT_CHANNEL].getValue());
        if (prevEditChannel != static_cast<int>(params[PARAM_EDIT_CHANNEL].getValue()))
        {
            for (int i = 0; i < 16; i++)
            {
                params[PARAM_GATE + i].setValue(getGate(channel, i) ? 10.F : 0.F);
            }
            prevEditChannel = static_cast<int>(params[PARAM_EDIT_CHANNEL].getValue());
        }
        else
        {
            for (uint_fast8_t i = 0; i < 16; i++)
            {
                setGate(channel, i, params[PARAM_GATE + i].getValue() > 0.F);
            }
        }

        assert(channel < constants::NUM_CHANNELS); // NOLINT
        const int start = this->start[channel];    // NOLINT
        const int length = this->length[channel];  // NOLINT
        const int max = this->max[channel];        // NOLINT

        for (int i = 0; i < 16; i++) // reset all lights
        {
            // black
            lights[LIGHTS_GATE + i].setBrightness(0.F);
        }

        bool light_on = false;

        // Special case length == num_lights
        if (length == num_lights)
        {
            for (int buttonIdx = 0; buttonIdx < num_lights; ++buttonIdx)
            {
                if (((inx) != nullptr) && (inx->inputs[channel].isConnected()))
                {
                    light_on = inx->inputs[channel].getNormalPolyVoltage(
                                   0, (buttonIdx % num_lights)) > 0.F;
                }
                else
                {
                    light_on = getGate(channel, buttonIdx % num_lights);
                }
                if (light_on)
                {
                    // gray
                    lights[LIGHTS_GATE + buttonIdx].setBrightness(1.F);
                }
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
                if (((inx) != nullptr) && (inx->inputs[channel].isConnected()))
                {
                    light_on = inx->inputs[channel].getNormalPolyVoltage(
                                   0, (buttonIdx % num_lights)) > 0.F;
                }
                else
                {
                    light_on = getGate(channel, buttonIdx % num_lights);
                }
                if (light_on)
                {
                    // gray
                    lights[LIGHTS_GATE + buttonIdx].setBrightness(0.2F);
                }

                // Increment the button index for the next iteration, wrapping
                // around with modulo
                buttonIdx = (buttonIdx + 1) % num_lights;
            }

            // Active lights
            buttonIdx = start;
            while (buttonIdx != ((end + 1) % num_lights))
            {
                if (((inx) != nullptr) && (inx->inputs[channel].isConnected()))
                {
                    light_on = inx->inputs[channel].getNormalPolyVoltage(
                                   0, (buttonIdx % num_lights)) > 0.F;
                }
                else
                {
                    light_on = getGate(channel, buttonIdx % num_lights);
                }
                // params[(PARAM_GATE + buttonIdx) % num_lights].getValue() >
                // 0.F;
                if (light_on)
                {
                    // white
                    lights[LIGHTS_GATE + buttonIdx].setBrightness(1.F);
                }

                // Increment the button index for the next iteration, wrapping
                // around with modulo
                buttonIdx = (buttonIdx + 1) % num_lights;
            }
        }
    }

    void updateRightExpanders()
    {
        outx = updateExpander<OutX, RIGHT>(rightAllowedModels);
        // DEBUG("Spike: updateRightExpanders() outx = %s", outx ? "aOutx" :
        // "nullptr");
    }
    void updateLeftExpanders()
    {
        inx = updateExpander<InX, LEFT>(leftAllowedModels);
        rex = updateExpander<ReX, LEFT>(leftAllowedModels);
        updatePolyphonySource();
    }

    void updatePolyphonySource()
    {
        if (polyphonySource == PHI)
        {
            return;
        }
        if (!inx && (polyphonySource == INX))
        {
            polyphonySource = PHI;
        }
        if ((!rex && ((polyphonySource == REX_CV_LENGTH) || polyphonySource == REX_CV_START)))
        {
            polyphonySource = PHI;
        }
    }

    // int getSequenceStart(int channel)
    // {

    // }
    struct StartLenMax
    {
        int start = 0;
        int length = 16;
        int max = 16;
    };
    StartLenMax updateStartLenMax(int channel)
    {
        StartLenMax retVal;
        if (!rex && !inx)
        {
            // XXX YYY REMOVE AFTER REFACTORING
            start[channel] = retVal.start;
            length[channel] = retVal.length;
            max[channel] = retVal.max;
            return retVal;
        }

        const int inx_channels = inx ? inx->inputs[channel].getChannels() : NUM_CHANNELS;
        retVal.max = inx_channels == 0 ? NUM_CHANNELS : inx_channels;

        if (!rex && inx)
        {
            retVal.length = retVal.max;
            // XXX YYY REMOVE AFTER REFACTORING
            start[channel] = retVal.start;
            length[channel] = retVal.length;
            max[channel] = retVal.max;
            return retVal;
        }
        // rex is connected
        const float rex_start_cv_input =
            rex->inputs[ReX::INPUT_START].getNormalPolyVoltage(0, channel);
        const float rex_length_cv_input =
            rex->inputs[ReX::INPUT_LENGTH].getNormalPolyVoltage(1, channel);
        const int rex_param_start = static_cast<int>(rex->params[ReX::PARAM_START].getValue());
        const int rex_param_length = static_cast<int>(rex->params[ReX::PARAM_LENGTH].getValue());

        const bool rex_start_cv_connected = rex->inputs[ReX::INPUT_START].isConnected();
        const bool rex_length_cv_connected = rex->inputs[ReX::INPUT_LENGTH].isConnected();

        // const int adjusted_maxLength = retVal.max > 0 ? patternLength : maxLength;
        retVal.start = rex_start_cv_connected ?
                                              // Clamping prevents out of range values due
                                              // to CV smaller than 0 and bigger than 10
                           clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0,
                                                          static_cast<float>(retVal.max))),
                                 0, retVal.max - 1)
                                              : clamp(rex_param_start, 0, retVal.max - 1);
        // YYY
        //    : rex_param_start;
        retVal.length = rex_length_cv_connected ?
                                                // Clamping prevents out of range values due
                                                // to CV smaller than 0 and bigger than 10
                            clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1,
                                                           static_cast<float>(retVal.max + 1))),
                                  1, retVal.max)
                                                : clamp(rex_param_length, 1, retVal.max);
        // YYY
        //  : rex_param_length;
        // XXX YYY REMOVE AFTER REFACTORING
        start[channel] = retVal.start;
        length[channel] = retVal.length;
        max[channel] = retVal.max;

        return retVal;
    }
    void processChannel(const ProcessArgs &args, int channel, int channelCount, bool ui_update)
    {
        StartLenMax startLenMax = updateStartLenMax(channel);

        const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, channel);
        const bool change = (cv != prevCv[channel]); // NOLINT
        const float phase = connectEnds ? clamp(cv / 10.F, 0.F, 1.F)
                                        : clamp(cv / 10.F, 0.00001F,
                                                .99999f); // to avoid jumps at 0 and 1
        const bool direction = getDirection(cv, prevCv[channel]);
        prevCv[channel] = cv;
        const int channel_index =
            (((clamp(static_cast<int>(std::floor(static_cast<float>(startLenMax.length) * (phase))),
                     0, startLenMax.length)) +
              startLenMax.start)) %
            (startLenMax.max);

        if ((channel == static_cast<int>(params[PARAM_EDIT_CHANNEL].getValue())) && ui_update)
        {
            updateUi(inx, startLenMax.max);
        }

        const bool channelChanged = (prevChannelIndex[channel] != channel_index) && change;
        const bool memoryGate = getGate(channel, channel_index); // NOLINT
        // const int maximum = inx != nullptr ? inx->inputs[channel].getChannels() : maxLength;
        const bool inxOverWrite = inx && (inx->inputs[channel].getChannels() > 0);
        const bool inxGate =
            inxOverWrite &&
            (inx->inputs[channel].getNormalPolyVoltage(0, (channel_index % startLenMax.max)) > 0);
        // YYY re enable?
        //  if (inxGate || memoryGate)
        {
            const float adjusted_duration =
                params[PARAM_DURATION].getValue() * 0.1F *
                inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, channel_index);
            gateMode.triggerGate(channel, adjusted_duration, phase, length[channel], direction);
        }
        if (channelChanged) // change bool to assure we don't trigger if ReX
                            // is modifying us
        {
            // XXX MAYBE we should check if anything between
            // prevChannelIndex +/- 1 and channel_index is set so we can
            // trigger those gates too when the input signal jumps (or have
            // it as an option)

            prevChannelIndex[channel] = channel_index;
        }

        const bool processGate = gateMode.process(channel, phase, args.sampleTime);
        const bool gate = processGate && (inxOverWrite ? inxGate : memoryGate);
        bool snooped = false;
        if (outx != nullptr)
        {
            const bool channel_connected = outx->outputs[channel_index].isConnected();
            if (channel_connected)
            {
                outx->outputs[channel_index].setChannels(channelCount);
            }
            snooped = outx->setExclusiveOutput(channel_index, gate ? 10.F : 0.F, channel) && gate;
        }
        outputs[OUTPUT_GATE].setVoltage(snooped ? 0.F : (gate ? 10.F : 0.F), channel);
    }

    void process(const ProcessArgs &args) override
    {
        const bool rex_start_cv_connected =
            (rex != nullptr) && rex->inputs[ReX::INPUT_START].isConnected();
        const bool rex_length_cv_connected =
            (rex != nullptr) && rex->inputs[ReX::INPUT_LENGTH].isConnected();
        const int phaseChannelCount = inputs[INPUT_CV].getChannels();
        // XXX TODO outputs .. to max(phaseChannelCount or highest inx connected
        // port.
        outputs[OUTPUT_GATE].setChannels(phaseChannelCount);
        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;
        // const bool start_len_update =
        //     uiTimer.process(args.sampleTime) > constants::START_LEN_UPDATE_TIME;
        int maxLength = NUM_CHANNELS;

        // Start runs from 0 to 15 (index)
        // int start = 0;
        // Length runs from 1 to 16 (count) and never zero
        // int length = 16;

        // XXX Move outside process()?
        // No InOut param
        auto calc_start_and_length =
            [this, &rex_start_cv_connected, &rex_length_cv_connected,
             &maxLength](int channel, int *start, int *length, int *maximum_length)
        {
            if (rex != nullptr)
            {
                const float rex_start_cv_input =
                    rex->inputs[ReX::INPUT_START].getNormalPolyVoltage(0, channel);
                const float rex_length_cv_input =
                    rex->inputs[ReX::INPUT_LENGTH].getNormalPolyVoltage(1, channel);
                const int rex_param_start =
                    static_cast<int>(rex->params[ReX::PARAM_START].getValue());
                const int rex_param_length =
                    static_cast<int>(rex->params[ReX::PARAM_LENGTH].getValue());

                if (inx == nullptr)
                {
                    *start = rex_start_cv_connected ?
                                                    // Clamping prevents out of range values due to
                                                    // CV smaller than 0 and bigger than 10
                                 clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0,
                                                                static_cast<float>(maxLength))),
                                       0, maxLength - 1)
                                                    : rex_param_start;
                    *length =
                        rex_length_cv_connected ?
                                                // Clamping prevents out of range values due
                                                // to CV smaller than 0 and bigger than 10
                            clamp(static_cast<int>(rescale(rex_length_cv_input, 0, 10, 1,
                                                           static_cast<float>(maxLength + 1))),
                                  1, maxLength)
                                                : rex_param_length;
                    *maximum_length = 16;
                }
                else // inx connected
                {
                    const int patternLength =
                        inx->inputs[channel].getChannels(); // Phase channels in Spike
                                                            // correspond to nth port in InX
                    const int adjusted_maxLength = patternLength > 0 ? patternLength : maxLength;
                    *start =
                        rex_start_cv_connected ?
                                               // Clamping prevents out of range values due to CV
                                               // smaller than 0 and bigger than 10
                            clamp(static_cast<int>(rescale(rex_start_cv_input, 0, 10, 0,
                                                           static_cast<float>(adjusted_maxLength))),
                                  0, adjusted_maxLength - 1)
                                               :
                                               // Clamping prevents out of range values due smaller
                                               // than 16 range due to pattern length
                            clamp(rex_param_start, 0, adjusted_maxLength - 1);
                    *length = rex_length_cv_connected ?
                                                      // Clamping prevents out of range values due
                                                      // to CV smaller than 0 and bigger than 10
                                  clamp(static_cast<int>(
                                            rescale(rex_length_cv_input, 0, 10, 1,
                                                    static_cast<float>(adjusted_maxLength + 1))),
                                        1, adjusted_maxLength)
                                                      :
                                                      // Clamping prevents out of range values due
                                                      // smaller than 16 range due to pattern length
                                  clamp(rex_param_length, 0, adjusted_maxLength);
                    *maximum_length = adjusted_maxLength;
                }
            }
            else
            {
                *start = 0;
                if (inx == nullptr)
                {
                    *length = maxLength;
                    *maximum_length = maxLength;
                }
                else // inx connected but no rex
                {
                    const int channels = inx->inputs[channel].getChannels();
                    *length = channels > 0 ? channels : maxLength;
                    *maximum_length = *length;
                }
            }
        };

        if (ui_update)
        {
            uiTimer.reset();
            if (phaseChannelCount == 0) // no input connected. Update ui
            {
                const int editchannel = static_cast<int>(getParam(PARAM_EDIT_CHANNEL).getValue());
                int inx_channels = 0;
                calc_start_and_length(editchannel, &start[editchannel],     // NOLINT
                                      &length[editchannel], &inx_channels); // NOLINT
                maxLength = inx_channels != 0 ? inx_channels : 16;
                updateUi(inx, maxLength);
            }
        }
        if ((((polyphonySource == REX_CV_LENGTH) && !rex_length_cv_connected) ||
             ((polyphonySource == REX_CV_START) && !rex_start_cv_connected)) ||
            !rex)
        {
            polyphonySource = PHI;
        }
        switch (polyphonySource)
        {
        case PHI:
        {
            for (int curr_channel = 0; curr_channel < phaseChannelCount; ++curr_channel)
            {
                processChannel(args, curr_channel, phaseChannelCount, ui_update);
            }
        }
        default:
            break;
        }
        return;
        for (int phaseChannel = 0; phaseChannel < phaseChannelCount; phaseChannel++)
        {
            // processChannel(args, phaseChannel);
            // XXX MOve everything below to processChannel()

            int inx_channels = 0;
            assert(phaseChannel < constants::NUM_CHANNELS);           // NOLINT
            calc_start_and_length(phaseChannel, &start[phaseChannel], // NOLINT
                                  &length[phaseChannel],
                                  &inx_channels); // NOLINT
            // calc_start_and_length(phaseChannel, &start[phaseChannel],
            // &length[phaseChannel]);

            const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, phaseChannel);
            const bool change = (cv != prevCv[phaseChannel]); // NOLINT
            const float phase = connectEnds ? clamp(cv / 10.F, 0.F, 1.F)
                                            : clamp(cv / 10.F, 0.00001F,
                                                    .99999f); // to avoid jumps at 0 and 1
            const bool direction = getDirection(cv, prevCv[phaseChannel]);
            prevCv[phaseChannel] = cv;
            const int channel_index =
                (((clamp(static_cast<int>(
                             std::floor(static_cast<float>(length[phaseChannel]) * (phase))),
                         0, length[phaseChannel])) +
                  start[phaseChannel])) %
                (maxLength);

            if ((phaseChannel == static_cast<int>(params[PARAM_EDIT_CHANNEL].getValue())) &&
                ui_update)
            {
                // before change
                // XXX TODO put next two lines in the right place
                //  const int inx_channels = inx ?
                //  inx->inputs[phaseChannel].getChannels() : 0; maxLength =
                //  inx_channels ? inx_channels : 16; updateUi(inx, maxLength);
                updateUi(inx, inx_channels);
            }

            const bool channelChanged = (prevChannelIndex[phaseChannel] != channel_index) && change;
            const bool memoryGate = getGate(phaseChannel, channel_index); // NOLINT
            const int maximum =
                inx != nullptr ? inx->inputs[phaseChannel].getChannels() : maxLength;
            const bool inxOverWrite =
                (inx != nullptr) && (inx->inputs[phaseChannel].getChannels() > 0);
            const bool inxGate =
                inxOverWrite &&
                (inx->inputs[phaseChannel].getNormalPolyVoltage(0, (channel_index % maximum)) > 0);
            if (channelChanged) // change bool to assure we don't trigger if ReX
                                // is modifying us
            {
                // XXX MAYBE we should check if anything between
                // prevChannelIndex +/- 1 and channel_index is set so we can
                // trigger those gates too when the input signal jumps (or have
                // it as an option)
                if (inxGate || ((inx == nullptr) && memoryGate))
                {
                    const float adjusted_duration =
                        params[PARAM_DURATION].getValue() * 0.1F *
                        inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, channel_index);
                    gateMode.triggerGate(phaseChannel, adjusted_duration, phase,
                                         length[phaseChannel], direction);
                }
                prevChannelIndex[phaseChannel] = channel_index;
            }

            const bool processGate = gateMode.process(phaseChannel, phase, args.sampleTime);
            const bool gate = processGate && (inxOverWrite ? inxGate : memoryGate);
            bool snooped = false;
            if (outx != nullptr)
            {
                const bool channel_connected = outx->outputs[channel_index].isConnected();
                if (channel_connected)
                {
                    outx->outputs[channel_index].setChannels(phaseChannelCount);
                }
                snooped =
                    outx->setExclusiveOutput(channel_index, gate ? 10.F : 0.F, phaseChannel) &&
                    gate;
            }
            outputs[OUTPUT_GATE].setVoltage(snooped ? 0.F : (gate ? 10.F : 0.F), phaseChannel);
        }
    };

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "gateMode", json_integer(gateMode.getGateMode()));
        json_object_set_new(rootJ, "singleMemory", json_integer(singleMemory));
        json_object_set_new(rootJ, "connectEnds",
                            json_integer(static_cast<json_int_t>(connectEnds)));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ != nullptr)
        {
            gateMode.setGateMode(static_cast<GateMode::Modes>(json_integer_value(gateModeJ)));
        };
        json_t *singleMemoryJ = json_object_get(rootJ, "singleMemory");
        if (singleMemoryJ != nullptr)
        {
            singleMemory = static_cast<int>(json_integer_value(singleMemoryJ));
        };
        json_t *connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ != nullptr)
        {
            connectEnds = (json_integer_value(connectEndsJ) != 0);
        };
    };
};

using namespace dimensions; // NOLINT
struct SpikeWidget : ModuleWidget
{
    template <typename TLight> struct SIMLightLatch : VCVLightLatch<TLight>
    {
        SIMLightLatch()
        {
            this->momentary = false;
            this->latch = true;
            this->frames.clear();
            this->addFrame(
                Svg::load(asset::plugin(pluginInstance, "res/components/SIMLightButton_0.svg")));
            this->addFrame(
                Svg::load(asset::plugin(pluginInstance, "res/components/SIMLightButton_1.svg")));
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
                this->shadow->box.pos =
                    center.minus(this->shadow->box.size.div(2).plus(math::Vec(0, -1.5F)));
            }
        }
    };

    explicit SpikeWidget(Spike *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::INPUT_CV));
        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::OUTPUT_GATE));

        addChild(createSegment2x8Widget<Spike>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data
            {
                const int editChannel =
                    static_cast<int>(module->params[Spike::PARAM_EDIT_CHANNEL].getValue());
                const int channels =
                    module->inx != nullptr ? module->inx->inputs[editChannel].getChannels() : 16;
                const int maximum = channels > 0 ? channels : 16;
                const int active = module->prevChannelIndex[editChannel] % maximum;
                // struct Segment2x8Data segmentdata = {module->start[editChannel],
                //                                      module->length[editChannel],
                //                                      module->max[editChannel], active};
                // YYY
                struct Segment2x8Data segmentdata = {module->start[editChannel],
                                                     module->length[editChannel], maximum, active};
                return segmentdata;
            }));

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(
                    mm2px(Vec(HP + (i * 2 * HP),
                              JACKYSTART + (j)*JACKYSPACE)), // NOLINT
                    module, Spike::PARAM_GATE + (j + i * 8), Spike::LIGHTS_GATE + (j + i * 8)));
            }
        }

        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, Spike::PARAM_DURATION));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module,
                                              Spike::INPUT_DURATION_CV));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, LOW_ROW)), module,
                                              Spike::PARAM_EDIT_CHANNEL));

        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module,
            Spike::LIGHT_LEFT_CONNECTED));
        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module,
            Spike::LIGHT_RIGHT_CONNECTED));
    }
    void draw(const DrawArgs &args) override { ModuleWidget::draw(args); }

    void appendContextMenu(Menu *menu) override
    {
        auto *module = dynamic_cast<Spike *>(this->module);
        assert(module); // NOLINT

        menu->addChild(new MenuSeparator); // NOLINT
        menu->addChild(createExpandableSubmenu(module, this));
        menu->addChild(new MenuSeparator); // NOLINT

        std::vector<std::string> operation_modes = {"One memory bank per Φ input",
                                                    "Single shared memory bank"};
        menu->addChild(createIndexSubmenuItem(
            "Gate Memory", operation_modes, [=]() -> int { return module->singleMemory; },
            [=](int index) { module->singleMemory = index; }));

        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
        menu->addChild(module->gateMode.createMenuItem());

        menu->addChild(createSubmenuItem(
            "Polyphony from",
            [=]() -> std::string
            {
                switch (module->polyphonySource)
                {
                case Spike::PolyphonySource::PHI:
                    return "Φ-in";
                case Spike::PolyphonySource::REX_CV_START:
                    return "ReX Start CV in";
                case Spike::PolyphonySource::REX_CV_LENGTH:
                    return "ReX Length CV in";
                case Spike::PolyphonySource::INX:
                    return "InX last port";
                default:
                    return "";
                }
                return "";
            }(),
            [=](Menu *menu)
            {
                menu->addChild(createMenuItem(
                    "Φ-in", module->polyphonySource == Spike::PHI ? "✔" : "",
                    [=]() { module->polyphonySource = Spike::PolyphonySource::PHI; }));
                MenuItem *rex_start_item = createMenuItem(
                    "ReX Start CV in",
                    module->polyphonySource == Spike::PolyphonySource::REX_CV_START ? "✔" : "",
                    [=]() { module->polyphonySource = Spike::PolyphonySource::REX_CV_START; });
                MenuItem *rex_length_item = createMenuItem(
                    "ReX Length CV in",
                    module->polyphonySource == Spike::PolyphonySource::REX_CV_LENGTH ? "✔" : "",
                    [=]() { module->polyphonySource = Spike::PolyphonySource::REX_CV_LENGTH; });
                if (module->rex == nullptr)
                {
                    rex_start_item->disabled = true;
                    rex_length_item->disabled = true;
                }
                if (module->rex && !module->rex->inputs[ReX::INPUT_START].isConnected())
                {
                    rex_start_item->disabled = true;
                }
                if (module->rex && !module->rex->inputs[ReX::INPUT_LENGTH].isConnected())
                {
                    rex_length_item->disabled = true;
                }
                menu->addChild(rex_start_item);
                menu->addChild(rex_length_item);

                MenuItem *inx_item = createMenuItem(
                    "InX Max Port",
                    module->polyphonySource == Spike::PolyphonySource::INX ? "✔" : "",
                    [=]() { module->polyphonySource = Spike::PolyphonySource::INX; });
                if (module->inx == nullptr)
                {
                    inx_item->disabled = true;
                }
                menu->addChild(inx_item);
            }));
    }
};

Model *modelSpike = createModel<Spike, SpikeWidget>("Spike"); // NOLINT