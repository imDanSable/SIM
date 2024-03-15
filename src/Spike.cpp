
// TODO: There's some waitfortriggerafterreset cruft that should be removed
// TODO: COlored lights
// TODO: Factor out relgate
// SOMEDAYMAYBE: Use https://github.com/bkille/BitLib/tree/master/include/bitlib/bitlib.h

#include <math.h>

#include <array>
#include <bitset>
#include <cassert>
#include <cmath>
#include <vector>
#include "InX.hpp"
#include "OutX.hpp"
#include "RelGate.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

#include "DSP/HCVTiming.h"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;

struct Spike : public biexpand::Expandable {
    enum ParamId { ENUMS(PARAM_GATE, MAX_GATES), PARAM_DURATION, PARAM_EDIT_CHANNEL, PARAMS_LEN };
    enum InputId { INPUT_CV, INPUT_RST, INPUT_DURATION_CV, INPUTS_LEN };
    enum OutputId { OUTPUT_GATE, OUTPUTS_LEN };
    enum LightId {
        ENUMS(LIGHTS_GATE, MAX_GATES),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

   private:
    friend struct SpikeWidget;

    // Used by Segment2x8
    struct StartLenMax {
        int start = {0};
        int length = {MAX_GATES};
        int max = {MAX_GATES};
    } channelProperties = {};

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;

    std::array<HCVPhasorStepDetector, MAX_GATES> stepDetectors;
    std::array<HCVPhasorSlopeDetector, MAX_GATES> slopeDetectors;
    std::array<dsp::BooleanTrigger, MAX_GATES> gateTriggers;

    int polyphonyChannels = 1;
    bool usePhasor = true;
    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    dsp::SchmittTrigger resetTrigger;
    std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutPulses = {};

    bool connectEnds = false;
    bool keepPeriod = false;

    std::array<int, MAX_GATES> prevGateIndex = {};
    int activeIndex = 0;

    RelGate relGate;

    // std::array<std::bitset<MAX_GATES>, MAX_BANKS> bitMemory = {};
    std::array<bool, MAX_GATES> bitMemory = {};

    std::vector<bool> v1, v2;
    std::array<std::vector<bool>*, 2> voltages{&v1, &v2};
    std::vector<bool>& readBuffer() const
    {
        return *voltages[0];
    }
    std::vector<bool>& readBuffer()
    {
        return *voltages[0];
    }
    std::vector<bool>& writeBuffer()
    {
        return *voltages[1];
    }
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }

    dsp::Timer uiTimer;

    float getDuration() const
    {
        return const_cast<float&>(params[PARAM_DURATION].value) * 0.01F;  // NOLINT
    }
    bool getGate(int gateIndex) const
    {
        assert(gateIndex < MAX_GATES);
        return bitMemory[gateIndex];
    };

    void setGate(int gateIndex, bool value)
    {
        bitMemory[gateIndex] = value;
    };

   public:
    Spike()
        : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(INPUT_CV, "Φ-in");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.1F, 100.F, 100.F, "Gate duration", "%", 0.F, 0.01F);
        configParam(PARAM_EDIT_CHANNEL, 0.F, 15.F, 0.F, "Edit Channel", "", 0.F, 1.F, 1.F);
        getParamQuantity(PARAM_EDIT_CHANNEL)->snapEnabled = true;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(PARAM_GATE + i, 0.0F, 1.0F, 0.0F, "Gate " + std::to_string(i + 1));
        }
        voltages[0]->resize(MAX_GATES);
        voltages[1]->resize(MAX_GATES);
    }

    void onReset() override
    {
        prevGateIndex.fill(0);
        bitMemory.fill(false);
        connectEnds = false;

        // prevEditChannel = 0;
        relGate.reset();
        channelProperties = {};
        prevGateIndex = {};
        params[PARAM_EDIT_CHANNEL].setValue(0.F);
        params[PARAM_DURATION].setValue(100.F);
        for (int i = 0; i < MAX_GATES; i++) {
            params[PARAM_GATE + i].setValue(0.F);
        }
    }

    void processPhasor(const ProcessArgs& args, int channel)
    {
        const float numSteps = readBuffer().size();
        const float pulseWidth = getDuration();
        const float phasorIn = inputs[INPUT_CV].getNormalPolyVoltage(0, channel);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        stepDetectors[channel].setNumberSteps(numSteps);
        stepDetectors[channel].setMaxSteps(MAX_GATES);
        /*const bool triggered = */ stepDetectors[channel](normalizedPhasor);
        const int currentIndex = (stepDetectors[channel].getCurrentStep());
        const float fractionalIndex = stepDetectors[channel].getFractionalStep();
        bool reversePhasor = slopeDetectors[channel](normalizedPhasor) < 0.0f;

        float gate = reversePhasor
                         ? (1.0f - fractionalIndex) < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f
                     : fractionalIndex < pulseWidth ? HCV_PHZ_GATESCALE
                                                    : 0.0f;

        outputs[OUTPUT_GATE].setVoltage(readBuffer()[currentIndex] ? gate : 0.0f, channel);

        activeIndex = (currentIndex + rex.getStart(channel, MAX_GATES)) % MAX_GATES;
    }

    void processTrigger(const ProcessArgs& args, bool ui_update)
    {
        int numChannels = getPolyCount();
        outputs[OUTPUT_GATE].setChannels(numChannels);

        const bool reset = checkReset();

        int gateIndex = {};
        for (int channel = 0; channel < numChannels; ++channel) {
            const StartLenMax startLenMax = getStartLenMax(channel);
            bool triggered = clockTracker[channel].process(
                args.sampleTime, inputs[INPUT_CV].getNormalPolyVoltage(0, channel));

            if (reset) { gateIndex = rex.getStart(channel, NUM_CHANNELS); }
            else {
                gateIndex = prevGateIndex[channel];
            }
            if (triggered) {
                if (gateIndex >= startLenMax.start) {
                    gateIndex = ((((gateIndex + 1) - (startLenMax.start)) % (startLenMax.length)) +
                                 startLenMax.start) %
                                startLenMax.max;
                }
                else {
                    gateIndex = ((((gateIndex + startLenMax.max + 1) - (startLenMax.start)) %
                                  (startLenMax.length)) +
                                 startLenMax.start) %
                                startLenMax.max;
                }
            }
            // Determine if the gate is on given the memory options
            const bool memoryGate = getGate(gateIndex);  // NOLINT

            // Are we on a new gate?
            if ((prevGateIndex[channel] != gateIndex) || triggered) {
                // Is this new gate high?
                if (memoryGate) {  // TEST: If memorygate=1 and inxGate overwrite=0, we
                                   // should get no gate. I think this code is wrong
                    // Calculate the relative duration
                    const float relative_duration =
                        params[PARAM_DURATION].getValue() * 0.1F *
                        inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, gateIndex);
                    // Update the relative gate class with the new duration
                    {
                        // we add a little delta 1e-3F to the duriation to avoid jitter at 100%
                        // XXX: Maybe we should add args.sampleTime*constant instead of 1e-3F
                        // to be independent of the sample rate
                        relGate.triggerGate(channel, relative_duration,
                                            clockTracker[channel].getPeriod() + 1e-3F);
                    }
                }
                // Update the gate index
                prevGateIndex[channel] = gateIndex;
            }

            // Update the relative gate class with the new phase or
            bool processGate = false;
            processGate = relGate.processTrigger(channel, args.sampleTime);
            // Determine if the gate is on
            const bool gateHigh =
                processGate && (memoryGate);  // XXX maybe this can go to: Is this new gate high
            // Determine if the gate is cut
            bool gateCut = false;  // XXX maybe this can go to: Is this new gate high
            if (outx.setChannels(numChannels, gateIndex)) {
                gateCut =
                    outx.setPortVoltage(gateIndex, gateHigh ? 10.F : 0.F, channel) && gateHigh;
            }
            // Set the gate output according the gateHigh and gateCut values
            outputs[OUTPUT_GATE].setVoltage(gateCut ? 0.F : (gateHigh ? 10.F : 0.F), channel);

            if (ui_update) {
                channelProperties = getStartLenMax(channel);
                activeIndex = prevGateIndex[channel];
                updateUi();
            }
        }
    }
    void readVoltages(int bank)
    {
        // Ensure the bankth vector in voltages[0][bank] has enough elements
        voltages[0][bank].resize(MAX_GATES);

        // Copy bitMemory to voltages[0][bank]
        std::copy(bitMemory.begin(), bitMemory.end(), voltages[0]->begin());
    }
    void writeVoltages(int channel)
    {
        // This should be  something like: outputs[OUTPUT_GATE].writeVoltages(writeBuffer().data());
        // outputs[OUTPUT_GATE].setVoltage(writeBuffer()[channel] ? 10.F : 0.F, channel);
    }

    template <typename Adapter>  // Double
    void perform_transform(Adapter& adapter, int channel)
    {
        if (adapter) {
            writeBuffer().resize(16);
            auto newEnd = adapter.transform(readBuffer().begin(), readBuffer().end(),
                                            writeBuffer().begin(), channel);
            const int channels = std::distance(writeBuffer().begin(), newEnd);
            writeBuffer().resize(channels);
            swap();
        }
    }
    static void debugBuff(std::vector<bool>& buff)
    {
        std::string s;
        for (bool b : buff) {
            s += b ? "1" : "0";
        }
        DEBUG("buff: %s", s.c_str());
    }

    // void debugDump()
    // {
    //     std::string s = "\n";
    //     for (int i = 0; i < getPolyCount(); i++) {
    //         for (bool b : bitMemory[i]) {
    //             s += b ? "1" : "0";
    //         }
    //         s += "\n";
    //     }
    //     DEBUG("buff: %s", s.c_str());
    // }

    void process(const ProcessArgs& args) override
    {
        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;
        // if (ui_update) {
        //     uiTimer.reset();
        //     if (polyphonyChannels == 0)  // no input connected. Update ui
        //     {
        //         updateUi(getStartLenMax(0));  // XXX Watch out. This 0 is during refactor
        //     }
        // }
        const int numChannels = getPolyCount();
        readVoltages(0);
        for (int channel = 0; channel < numChannels; channel++) {
            outputs[OUTPUT_GATE].setChannels(numChannels);
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                perform_transform(*adapter, channel);
            }
            if (channel) {
                // debugDump();
                // debugBuff(readBuffer());
            }

            if (usePhasor) { processPhasor(args, channel); }
            else {
                processTrigger(args, ui_update);
            }
            writeVoltages(channel);
        }

        if (ui_update) {
            // channelProperties = getStartLenMax(0);
            channelProperties.start = rex.getStart();
            channelProperties.length = readBuffer().size();
            channelProperties.max = MAX_GATES;
            updateUi();
        }
        // processTrigger(args, ui_update);
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_integer(connectEnds));
        json_object_set_new(rootJ, "keepPeriod", json_integer(keepPeriod));
        json_object_set_new(rootJ, "polyphonyChannels", json_integer(polyphonyChannels));
        json_t* gateJ = json_array();
        for (int i = 0; i < MAX_GATES; i++) {
            json_array_append_new(gateJ, json_boolean(static_cast<bool>(bitMemory[i])));
        }
        json_object_set_new(rootJ, "gates", gateJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ != nullptr) { connectEnds = (json_integer_value(connectEndsJ) != 0); };
        json_t* keepPeriodJ = json_object_get(rootJ, "keepPeriod");
        if (keepPeriodJ != nullptr) { keepPeriod = (json_integer_value(keepPeriodJ) != 0); };
        json_t* polyphonyChannelsJ = json_object_get(rootJ, "polyphonyChannels");
        if (polyphonyChannelsJ != nullptr) {
            polyphonyChannels = static_cast<int>(json_integer_value(polyphonyChannelsJ));
        };
        // And read the gates array from the JSON object
        json_t* gatesJ = json_object_get(rootJ, "gates");
        if (gatesJ) {
            for (int i = 0; i < MAX_GATES; i++) {
                json_t* gateJ = json_array_get(gatesJ, i);
                if (gateJ) { bitMemory[i] = json_boolean_value(gateJ); }
            }
        }
    };

   private:
    bool checkReset()
    {
        const bool resetConnected = inputs[INPUT_RST].isConnected();
        if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            for (int i = 0; i < NUM_CHANNELS; ++i) {
                clockTracker[i].init(keepPeriod ? clockTracker[i].getPeriod() : 0.F);
                trigOutPulses[i].reset();
                prevGateIndex[i] = rex.getStart(i, MAX_GATES);

                if (getGate(rex.getStart(i))) {
                    // Trigger the gate
                    // TEST if gatelength is correct
                    trigOutPulses[i].trigger(params[PARAM_DURATION].getValue() * 0.01F *
                                             clockTracker[i].getPeriod());
                }
            }
            return true;
        }
        return false;
    }

    int getPolyCount() const
    {
        return polyphonyChannels;
    }

    void memToParam()
    {
        for (int i = 0; i < MAX_GATES; i++) {
            params[i].setValue(getGate(i) ? 1.F : 0.F);
        }
    }

    void paramToMem()
    {
        for (int i = 0; i < MAX_GATES; i++) {
            setGate(i, params[PARAM_GATE + i].getValue() > 0.F);
        }
    }

    void updateUi()
    {
        std::array<float, MAX_GATES> brightnesses{};
        paramToMem();
        const int start = rex.getStart();
        const int length = readBuffer().size();
        // use a lambda to get the gate
        auto getBufGate = [this, start, length](int gateIndex) {
            return (readBuffer()[gateIndex % MAX_GATES]);
        };
        auto getBitGate = [this](int gateIndex) { return getGate(gateIndex % MAX_GATES); };
        const int max = MAX_GATES;

        if (length == max) {
            for (int i = 0; i < max; ++i) {
                lights[LIGHTS_GATE + i].setBrightness(getBufGate(i));
            }
            return;
        }
        for (int i = 0; i < max; ++i) {
            getBitGate(i);
            if (getBitGate(i)) { brightnesses[(i) % max] = 0.2F; }
        }
        for (int i = 0; i < length; ++i) {
            if (getBufGate(i)) { brightnesses[(i + start)] = 1.F; }
        }
        for (int i = 0; i < MAX_GATES; ++i) {
            lights[LIGHTS_GATE + i].setBrightness(brightnesses[i]);
        }
    }

    StartLenMax getStartLenMax(int channel) const
    {
        StartLenMax retVal;
        if (!rex && !inx) {
            return retVal;  // 0, MAX_GATES, MAX_GATES
        }
        // const int inx_channels = inx.getNormalChannels(NUM_CHANNELS, channel);
        // retVal.max = inx_channels == 0 ? NUM_CHANNELS : inx_channels;
        retVal.max = NUM_CHANNELS;

        if (!rex && inx) {
            /// XXX Check/test/decide if next line is correct
            retVal.length = retVal.max;
            return retVal;
        }
        retVal.start = rex.getStart(channel, retVal.max);
        retVal.length = rex.getLength(channel, retVal.max);
        return retVal;
    }
};
using namespace dimensions;  // NOLINT
struct SpikeWidget : ModuleWidget {
    explicit SpikeWidget(Spike* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::INPUT_CV));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::INPUT_RST));

        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW - JACKNTXT)), module,
                                               Spike::OUTPUT_GATE));

        addChild(createSegment2x8Widget<Spike>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                const int maximum = MAX_GATES;
                const int active = module->activeIndex;
                struct Segment2x8Data segmentdata = {module->channelProperties.start,
                                                     module->channelProperties.length, maximum,
                                                     active};
                return segmentdata;
            }));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(
                    mm2px(Vec(HP + (i * 2 * HP),
                              JACKYSTART + (j)*JACKYSPACE)),  // NOLINT
                    module, Spike::PARAM_GATE + (j + i * 8), Spike::LIGHTS_GATE + (j + i * 8)));
            }
        }

        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, Spike::PARAM_DURATION));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module,
                                              Spike::INPUT_DURATION_CV));

        addParam(createParamCentered<SIMEncoder>(mm2px(Vec(3 * HP, LOW_ROW)), module,
                                                 Spike::PARAM_EDIT_CHANNEL));
        auto* display = new LCDWidget();

        display->box.pos = Vec(35, 318);
        display->box.size = Vec(20, 21);
        display->offset = 1;
        display->textGhost = "18";
        if (module) {
            // XXX have value point to an int pointer
            // display->value = &module->prevEditChannel;
        }

        addChild(display);

        if (module) {
            module->addDefaultConnectionLights(this, Spike::LIGHT_LEFT_CONNECTED,
                                               Spike::LIGHT_RIGHT_CONNECTED);
        }
    }
    void draw(const DrawArgs& args) override
    {
        ModuleWidget::draw(args);
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Spike*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT

        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));

        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
        menu->addChild(createBoolPtrMenuItem("Keep Period after Reset", "", &module->keepPeriod));

        std::vector<std::string> polyphony_modes = {"1", "2",  "3",  "4",  "5",  "6",  "7",  "8",
                                                    "9", "10", "11", "12", "13", "14", "15", "16"};

        menu->addChild(createIndexSubmenuItem(  // NOLINT
            "Polyphony Channels", polyphony_modes,
            [=]() -> int { return module->polyphonyChannels - 1; },
            [=](int index) { module->polyphonyChannels = index + 1; }));
    }
};

Model* modelSpike = createModel<Spike, SpikeWidget>("Spike");  // NOLINT