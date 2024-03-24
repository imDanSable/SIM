// TODO: Active index not visible in light panel
// TODO: Colored lights
// TODO: Glide
// XXX: ??? use writeBuffer() instead of outputs[OUTPUT_GATE].writeVoltages(writeBuffer().data());
// SOMEDAYMAYBE: Menu option Zero output when no phasor movement
// SOMEDAYMAYBE: Use https://github.com/bkille/BitLib/tree/master/include/bitlib/bitlib.

#include <array>
#include <cassert>
#include <cmath>
#include <vector>
#include "DebugX.hpp"
#include "GaitX.hpp"
#include "InX.hpp"
#include "ModX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

#include "DSP/Phasors/HCVPhasorAnalyzers.h"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;

struct Spike : public biexpand::Expandable<bool> {
    enum ParamId { ENUMS(PARAM_GATE, MAX_GATES), PARAM_DURATION, PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUT_NEXT, INPUT_RST, INPUT_DURATION_CV, INPUTS_LEN };
    enum OutputId { OUTPUT_GATE, OUTPUTS_LEN };
    enum LightId {
        ENUMS(LIGHTS_GATE, MAX_GATES),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

   private:
    friend struct SpikeWidget;
    struct SpikeParamQuantity : ParamQuantity {
        std::string getString() override
        {
            return ParamQuantity::getValue() > constants::BOOL_TRESHOLD ? "On" : "Off";
        };
    };

    /// @brief: is the current step modified?
    /// XXX this should be an array so that each channels have its modParams
    ModXAdapter::ModParams modParams{};

    // Used by Segment2x8
    struct StartLenMax {
        int start = {0};
        int length = {MAX_GATES};
        int max = {MAX_GATES};
    } channelProperties = {};

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;
    ModXAdapter modx;
    GaitXAdapter gaitx;
    DebugXAdapter debugx;

    std::array<HCVPhasorStepDetector, MAX_GATES> stepDetectors;
    std::array<HCVPhasorSlopeDetector, MAX_GATES> slopeDetectors;
    std::array<dsp::BooleanTrigger, MAX_GATES>
        gateTriggers;  // XXX Not used. But we might want to copy Hetrick's behavior in favor of
                       // the current behavior
    std::array<HCVPhasorGateDetector, MAX_GATES> subGateDetectors;
    std::array<HCVPhasorStepDetector, MAX_GATES> subStepDetectors;

    int polyphonyChannels = 1;
    bool usePhasor = true;
    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    std::array<dsp::TTimer<float>, NUM_CHANNELS> nextTimer = {};
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger nextTrigger;
    dsp::PulseGenerator resetPulse;  // ignore clock pulses when reset is high for 1ms
    // std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutXPulses = {};

    bool connectEnds = false;
    bool keepPeriod = false;

    std::array<int, MAX_GATES> prevSubStepIndex{};
    // std::array<int, MAX_GATES> prevGateIndex = {};
    int activeIndex = 0;

    // std::array<std::bitset<MAX_GATES>, MAX_BANKS> bitMemory = {};
    std::array<bool, MAX_GATES> bitMemory = {};
    std::array<bool, MAX_GATES> randomizedMemory = {};

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

    /// @brief: returns the normalized relative gate duration of step
    float getDuration(int step)
    {
        return params[PARAM_DURATION].value * 0.1F *
               inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, step);
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
        : Expandable<bool>(
              {{modelReX, &this->rex}, {modelInX, &this->inx}, {modelModX, &this->modx}},
              {{modelOutX, &this->outx}, {modelGaitX, &this->gaitx}, {modelDebugX, &this->debugx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(INPUT_DRIVER, "phasor or clock pulse in");
        configInput(INPUT_NEXT, "Trigger to advance to the next step");
        configInput(INPUT_RST, "Reset");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.01F, 1.F, 1.F, "Gate duration", "%", 0.F, 100.F);
        for (int i = 0; i < MAX_GATES; i++) {
            configParam<SpikeParamQuantity>(PARAM_GATE + i, 0.0F, 1.0F, 0.0F,
                                            "Gate " + std::to_string(i + 1));
        }
        voltages[0]->resize(MAX_GATES);
        voltages[1]->resize(MAX_GATES);
        for (int i = 0; i < MAX_GATES; i++) {
            subGateDetectors[i].setSmartMode(true);
        }
    }

    void onReset() override
    {
        // prevGateIndex.fill(0);
        bitMemory.fill(false);
        connectEnds = false;

        channelProperties = {};
        // prevGateIndex = {};
    }

    /// @returns: gateOn, triggered, step, fractional step, reverse direction
    std::tuple<bool, bool, int, float, bool> checkPhaseGate(int channel, float normalizedPhasor)
    {
        const float numSteps = readBuffer().size();
        stepDetectors[channel].setNumberSteps(numSteps);
        stepDetectors[channel].setMaxSteps(
            PORT_MAX_CHANNELS);  // TODO: Check ?? what if we have rex.length > polyIn?

        bool triggered{};
        bool eoc{};
        triggered = stepDetectors[channel](normalizedPhasor);
        eoc = stepDetectors[channel]
                  .getEndOfCycle();  // XXX We should make use of recieving eoc here because later
                                     // on we miscalculate it it (refactored)
        const int currentIndex = (stepDetectors[channel].getCurrentStep());
        const float pulseWidth = getDuration(currentIndex);
        const float fractionalIndex = stepDetectors[channel].getFractionalStep();
        bool reversePhasor = slopeDetectors[channel](normalizedPhasor) < 0.0f;

        bool gate = false;
        if (reversePhasor) { gate = (1.0f - fractionalIndex) < pulseWidth; }
        else {
            gate = fractionalIndex < pulseWidth;
        }
        const bool gateOn = readBuffer()[currentIndex] && gate;
        return std::make_tuple(gateOn, triggered, currentIndex, fractionalIndex, reversePhasor);
    }

    bool applyModParams(int channel, int step, bool gateOn, bool triggered, float fraction)
    {
        if (!modx) { return gateOn; }
        if (triggered) {  // We need to process triggered, even when gateOn is false
            if (modx) { modParams = modx.getParams(step); }

            if (modParams.prob < 1.0F) {
                randomizedMemory[step] = random::uniform() < modParams.prob;
            }

            if (modParams.reps > 1) {
                float duration = getDuration(step) - 2 * 1e-3F;
                subGateDetectors[channel].setGateWidth(std::max(duration, 1e-3F));
                /* bool triggered = */ subGateDetectors[channel](
                    fraction);  // Should trigger per definition
            }
        }
        if (gateOn) {
            if (modParams.prob < 1.0F) {
                gateOn = randomizedMemory[step];
                if (!gateOn) { return false; }
            }
            if (modParams.glide) {  // XXX Finish
            }
        }

        if (readBuffer()[step]) {  // Process even when gateOn is false, we might be in the
                                   // gateOff part of a gateOn
            if (modParams.reps > 1) {
                const float subFraction = fmodf(fraction * modParams.reps, 1.F);
                const bool subStepGateTrigger = subGateDetectors[channel](subFraction);
                return subStepGateTrigger;
            }
        }
        return gateOn;
    }

    void processPhasor(float normalizedPhasor, int channel)
    {
        // XXX There is cruft here from refactoring checkPhaseGate to return more than a single
        // bool change back to gateOn only or gateOn and triggered if needed
        // const float phasorIn = normalizedPhasor * 10.f;
        // stepDetectors[channel](normalizedPhasor);
        /// XXXXXXXXXXXXX This is where we left of.  we just set phasorIn to our new phasor and hope
        /// all is well. I think it scales down twice
        // inputs[INPUT_DRIVER].getNormalPolyVoltage(0, channel);
        bool outxGateOn{};
        bool outxTriggered{};
        float outxFraction{};
        int outxStep{};
        bool outxReverse{};
        if (outx) {
            // Pre cut: calculate if the gate is on given no cuts have taken place for outx
            std::tie(outxGateOn, outxTriggered, outxStep, outxFraction, outxReverse) =
                (checkPhaseGate(channel, normalizedPhasor));
            // Cut: perform the cut
            perform_transform(outx, channel);
        }
        // Post cut: calculate if the gate is on given the cuts have taken place for main out
        bool gateOn{};
        bool triggered{};
        float fraction{};
        int step{};
        bool reverse{};
        std::tie(gateOn, triggered, step, fraction, reverse) =
            checkPhaseGate(channel, normalizedPhasor);
        bool final = applyModParams(channel, step, gateOn, triggered, fraction);
        // Write main out
        outputs[OUTPUT_GATE].setVoltage(final ? 10.F : 0.0f, channel);
        const int currentIndex = (stepDetectors[channel].getCurrentStep());
        // Write outx
        outx.setPortGate(currentIndex, outxGateOn, channel);
        // Write gaitx
        gaitx.setPhi(fraction * 10.F, channel);
        gaitx.setStep(step);
        bool eoc = (triggered && (step == 0) && !reverse) ||
                   (triggered && (step == MAX_GATES - 1) && reverse);
        gaitx.setEOC(eoc * 10.F, channel);

        activeIndex = (currentIndex + rex.getStart(channel, MAX_GATES)) % MAX_GATES;
    }

    void readVoltages()
    {
        voltages[0]->resize(MAX_GATES);

        std::copy(bitMemory.begin(), bitMemory.end(), voltages[0]->begin());
    }
    void writeVoltages(int channel)
    {
        // This should be  something like:
        // outputs[OUTPUT_GATE].writeVoltages(writeBuffer().data());
        // outputs[OUTPUT_GATE].setVoltage(writeBuffer()[channel] ? 10.F : 0.F, channel);
    }

    template <typename Adapter>  // Double
    void perform_transform(Adapter& adapter, int /*channel*/)
    {
        if (adapter) {
            writeBuffer().resize(16);
            if (adapter.inPlace(readBuffer().size(), 0)) {
                // Profiler p("InPlace");
                adapter.transform(readBuffer().begin(), readBuffer().end(), 0);
            }
            else {
                // Profiler p("Copy");
                auto newEnd = adapter.transform(readBuffer().begin(), readBuffer().end(),
                                                writeBuffer().begin(), 0);
                const int outputLength = std::distance(writeBuffer().begin(), newEnd);
                writeBuffer().resize(outputLength);
                swap();
                assert((outputLength <= 16) && (outputLength >= 0));  // NOLINT
            }
        }
    }

    /// @brief Uses clock to calculate the phase in when triggers to advance
    /// @return the phase within the sequence
    float timeToPhase(const ProcessArgs& args,
                      int channel,
                      float cv)  // XXX 100% double code with phi
    {
        // update our nextTimer
        nextTimer[channel].process(args.sampleTime);

        // Update our clock tracker
        const bool ignoreClock = resetPulse.process(args.sampleTime);
        const bool isClockConnected = inputs[INPUT_DRIVER].isConnected();

        const bool isClockTriggered =  // We will need the results when we normal the inputs
            isClockConnected ? clockTracker[channel].process(args.sampleTime, cv) && !ignoreClock
                             : false;
        const bool isNextConnected = inputs[INPUT_NEXT].isConnected();
        const bool isNextNormalled = isClockConnected && !isNextConnected;
        const int numSteps = readBuffer().size();
        // update our next trigger normalled to the clock if not connected
        const bool isNextTriggered =
            isNextConnected ? nextTrigger.process(inputs[INPUT_NEXT].getNormalVoltage(0.F, channel))
                            : isClockTriggered;
        if (isNextTriggered) {
            float period =
                isNextNormalled ? nextTimer[channel].getTime() : clockTracker[channel].getPeriod();
            nextTimer[channel].reset();
            clockTracker[channel].init(period);
        }
        const float stepFraction =
            clockTracker[channel].isPeriodDetected()
                ? nextTimer[channel].getTime() / clockTracker[channel].getPeriod()
                : 0.F;
        const int curStep = stepDetectors[channel].getCurrentStep();

        float phase = fmodf(
            static_cast<float>(isNextTriggered + curStep + clamp(stepFraction, 0.F, 0.9999F)) /
                numSteps,
            1.F);
        return phase;
    }
    void process(const ProcessArgs& args) override
    {
        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;
        const int numChannels = getPolyCount();
        readVoltages();
        if (!usePhasor) { checkReset(); }
        const int numSteps = readBuffer().size();
        if (numSteps == 0) {
            writeBuffer().resize(0);
            return;
        }
        for (int channel = 0; channel < numChannels; channel++) {
            outputs[OUTPUT_GATE].setChannels(numChannels);
            const float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                perform_transform(*adapter, channel);
            }
            // if (outx) { perform_transform(outx, channel); }
            const float normalizedPhasor =
                !usePhasor ? timeToPhase(args, channel, curCv) : scaleAndWrapPhasor(curCv);
            processPhasor(normalizedPhasor, channel);
            writeVoltages(channel);
        }
        gaitx.setChannels(numChannels);

        if (ui_update) { updateUi(); }
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
    void checkReset()  // 100% double code with Phi
    {
        const bool resetConnected = inputs[INPUT_RST].isConnected();
        if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            resetPulse.trigger(1e-3F);  // ignore clock for 1ms after reset
            nextTrigger.reset();
            for (int i = 0; i < NUM_CHANNELS; ++i) {
                clockTracker[i].init(keepPeriod ? clockTracker[i].getPeriod() : 0.1F);
                nextTimer[i].reset();
                stepDetectors[i].setStep(0);
            }
        }
    }
    // bool checkReset2()
    // {
    //     const bool resetConnected = inputs[INPUT_RST].isConnected();
    //     if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
    //         resetPulse.trigger(1e-3F);  // Start ignoring clock pulses for 1ms
    //         nextTrigger.reset();
    //         for (int i = 0; i < NUM_CHANNELS; ++i) {
    //             clockTracker[i].init(keepPeriod ? clockTracker[i].getPeriod() : 0.1F);
    //             nextTimer[i].reset();
    //             stepDetectors[i].setStep(0);
    //             // trigOutXPulses[i].reset();
    //             // prevGateIndex[i] = rex.getStart(i, MAX_GATES);
    //         }
    //         return true;
    //     }
    //     return false;
    // }

    int getPolyCount() const
    {
        return polyphonyChannels;
    }

    void paramToMem()
    {
        for (int i = 0; i < MAX_GATES; i++) {
            setGate(i, params[PARAM_GATE + i].getValue() > 0.F);
        }
    }

    void updateUi()
    {
        channelProperties.start = rex.getStart();
        channelProperties.length = readBuffer().size();
        channelProperties.max = MAX_GATES;
        paramToMem();
        std::array<float, MAX_GATES> brightnesses{};
        const int start = rex.getStart();
        const int length = readBuffer().size();
        // use a lambda to get the gate
        auto getBufGate = [this](int gateIndex) { return (readBuffer()[gateIndex % MAX_GATES]); };
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
            if (getBitGate(i)) { brightnesses[i] = 0.2F; }
        }
        for (int i = 0; i < length; ++i) {
            if (getBufGate(i)) { brightnesses[(i + start) % max] = 1.F; }
        }
        for (int i = 0; i < max; ++i) {
            lights[LIGHTS_GATE + i].setBrightness(brightnesses[i]);
        }
    }
};
using namespace dimensions;  // NOLINT
struct SpikeWidget : ModuleWidget {
    explicit SpikeWidget(Spike* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/Spike.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/Spike.svg")));

        float y = 14.5F;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y += 0)), module, Spike::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Spike::INPUT_RST));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(2 * HP, y += JACKNTXT / 2)), module,
                                              Spike::INPUT_NEXT));

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
        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW - 7.F + JACKYSPACE)),
                                               module, Spike::OUTPUT_GATE));
        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW - 7.F)), module,
                                              Spike::PARAM_DURATION));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE - 7.F)), module,
                                              Spike::INPUT_DURATION_CV));

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
    }
};

Model* modelSpike = createModel<Spike, SpikeWidget>("Spike");  // NOLINT