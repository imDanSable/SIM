// TODO: Colored lights
// TODO: Relative duration mode option a la varigate
// XXX: ??? use writeBuffer() instead of outputs[OUTPUT_GATE].writeVoltages(writeBuffer().data());
// SOMEDAYMAYBE: Menu option Zero output when no phasor movement
// SOMEDAYMAYBE: Use https://github.com/bkille/BitLib/tree/master/include/bitlib/bitlib.

#include <array>
#include <bitset>
#include <cassert>
#include <cmath>
#include <vector>
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

#include "DSP/HCVTiming.h"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;

struct Spike : public biexpand::Expandable {
    enum ParamId { ENUMS(PARAM_GATE, MAX_GATES), PARAM_DURATION, PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUT_RST, INPUT_DURATION_CV, INPUTS_LEN };
    enum OutputId { OUTPUT_GATE, OUTPUTS_LEN };
    enum LightId {
        ENUMS(LIGHTS_GATE, MAX_GATES),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

   private:
    friend struct SpikeWidget;

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

    std::array<HCVPhasorStepDetector, MAX_GATES> stepDetectors;
    std::array<HCVPhasorSlopeDetector, MAX_GATES> slopeDetectors;
    std::array<dsp::BooleanTrigger, MAX_GATES>
        gateTriggers;  // XXX Not used. But we might want to copy Hetrick's behavior
    std::array<HCVPhasorGateDetector, MAX_GATES> subGateDetectors;
    std::array<HCVPhasorStepDetector, MAX_GATES> subStepDetectors;

    int polyphonyChannels = 1;
    bool usePhasor = true;
    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    dsp::SchmittTrigger resetTrigger;
    dsp::PulseGenerator resetPulse;  // ignore clock pulses when reset is high for 1ms
    std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutPulses = {};
    std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutXPulses = {};

    bool connectEnds = false;
    bool keepPeriod = false;

    std::array<int, MAX_GATES> prevSubStepIndex{};
    std::array<int, MAX_GATES> prevGateIndex = {};
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
        : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}, {modelModX, &this->modx}},
                     {{modelOutX, &this->outx}, {modelGaitX, &this->gaitx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(INPUT_DRIVER, "Φ-in");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.01F, 1.F, 1.F, "Gate duration", "%", 0.F, 100.F);
        for (int i = 0; i < MAX_GATES; i++) {
            configParam(PARAM_GATE + i, 0.0F, 1.0F, 0.0F, "Gate " + std::to_string(i + 1));
        }
        voltages[0]->resize(MAX_GATES);
        voltages[1]->resize(MAX_GATES);
        for (int i = 0; i < MAX_GATES; i++) {
            subGateDetectors[i].setSmartMode(true);
        }
    }

    void onReset() override
    {
        prevGateIndex.fill(0);
        bitMemory.fill(false);
        connectEnds = false;

        channelProperties = {};
        prevGateIndex = {};
    }

    /// @returns: gateOn, triggered, step, fractional step, reverse direction
    std::tuple<bool, bool, int, float, bool> checkPhaseGate(int channel, float phasorIn)
    {
        const float numSteps = readBuffer().size();
        // const float phasorIn = inputs[INPUT_CV].getNormalPolyVoltage(0, channel);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        stepDetectors[channel].setNumberSteps(numSteps);
        stepDetectors[channel].setMaxSteps(MAX_GATES);
        const bool triggered = stepDetectors[channel](normalizedPhasor);
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

        if (readBuffer()[step]) {  // Process even when gateOn is false, we might be in the gateOff
                                   // part of a gateOn
            if (modParams.reps > 1) {
                // const bool subStepTriggered = subStepDetectors[channel](fraction);
                const float subFraction = fmodf(fraction * modParams.reps, 1.F);
                const bool subStepGateTrigger = subGateDetectors[channel](subFraction);
                return subStepGateTrigger;
            }
        }
        return gateOn;
    }

    void processPhasor(const ProcessArgs& /*args*/, int channel)
    {
        // XXX There is cruft here from refactoring checkPhaseGate to return more than a single
        // bool change back to gateOn only or gateOn and triggered if needed
        const float phasorIn = inputs[INPUT_DRIVER].getNormalPolyVoltage(0, channel);
        bool outxGateOn{};
        bool outxTriggered{};
        float outxFraction{};
        int outxStep{};
        bool outxReverse{};
        if (outx) {
            // Pre cut: calculate if the gate is on given no cuts have taken place for outx
            std::tie(outxGateOn, outxTriggered, outxStep, outxFraction, outxReverse) =
                (checkPhaseGate(channel, phasorIn));
            // Cut: perform the cut
            perform_transform(outx, channel);
        }
        // Post cut: calculate if the gate is on given the cuts have taken place for main out
        bool gateOn{};
        bool triggered{};
        float fraction{};
        int step{};
        bool reverse{};
        std::tie(gateOn, triggered, step, fraction, reverse) = checkPhaseGate(channel, phasorIn);
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

    void processTrigger(const ProcessArgs& args, int channel)
    {
        bool ignoreClock = resetPulse.process(args.sampleTime);
        bool triggered =
            clockTracker[channel].process(args.sampleTime,
                                          inputs[INPUT_DRIVER].getNormalPolyVoltage(0, channel)) &&
            !ignoreClock;
        const int curStep = (prevGateIndex[channel] + triggered) % readBuffer().size();

        const float curStepPhase = clockTracker[channel].getTimeFraction();
        if (modParams.reps > 1 && readBuffer().at(curStep)) {
            const int curSubStep =
                clamp(static_cast<int>(modParams.reps * curStepPhase) % modParams.reps, 0,
                      modParams.reps - 1);

            if ((curSubStep != prevSubStepIndex[channel]) &&
                (curSubStep != 0)) {  // Don't trigger first substep.
                                      // Pass that on to if (triggered)
                                      // But it seems that this fix breaks reps == 2
                //  XXX decide if / reps is the right thing to do
                //  XXX We must check buffer!!!
                // (modParams.reps +1) so that there is space to hear the trigger.
                trigOutPulses[channel].trigger(
                    std::max(getDuration(curStep) * clockTracker[channel].getPeriod() /
                                     (modParams.reps + 1) +
                                 args.sampleTime,
                             1e-3F));
            }
            prevSubStepIndex[channel] = curSubStep;
        }
        bool outxGateOn{};
        if (outx) {
            // Pre cut: calculate if the gate is on given no cuts have taken place for outx
            outxGateOn = readBuffer()[curStep];
            // Cut: perform the cut
            perform_transform(outx, channel);
        }
        const bool gateOn = readBuffer()[curStep];
        bool eoc = false;
        if (triggered) {
            eoc = curStep < prevGateIndex[channel];
            modParams = modx.getParams(curStep);
            prevGateIndex[channel] = curStep;
            // XXX For now, glide in spike means the gate is on for the duration of the full
            // step. But maybe we need to look at the step ahead and not prolong the game if the
            // next step in the buffer is not high
            // We also added a delta extra. We saw glitches at the end. Should be in terms of
            // sampleTime instead of .001F
            // XXX for now glide overrides reps
            float duration = modParams.glide ? 1.001F : getDuration(curStep);
            if (modParams.reps > 1) { duration = duration / (modParams.reps + 1); }
            const bool dice = modParams.prob >= 1.0F || random::uniform() < modParams.prob;
            if (dice) {
                if (gateOn) {
                    trigOutPulses[channel].trigger(duration * clockTracker[channel].getPeriod() +
                                                   args.sampleTime);
                }
                if (outxGateOn) {
                    trigOutXPulses[channel].trigger(duration * clockTracker[channel].getPeriod() +
                                                    args.sampleTime);
                }
            }
        }
        const bool gateHigh = trigOutPulses[channel].process(args.sampleTime);
        outputs[OUTPUT_GATE].setVoltage(gateHigh ? 10.F : 0.F, channel);
        gaitx.setEOC(eoc * 10.F, channel);
        gaitx.setPhi(curStepPhase * 10.F, channel);
        gaitx.setStep(curStep);
        const bool outxGateHigh = trigOutXPulses[channel].process(args.sampleTime) & outxGateOn;
        outx.setPortGate(curStep, outxGateHigh, channel);

        activeIndex = (curStep + rex.getStart(channel, MAX_GATES)) % MAX_GATES;
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

    void process(const ProcessArgs& args) override
    {
        checkReset();
        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;
        const int numChannels = getPolyCount();
        readVoltages();
        for (int channel = 0; channel < numChannels; channel++) {
            outputs[OUTPUT_GATE].setChannels(numChannels);
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                perform_transform(*adapter, channel);
            }
            // if (outx) { perform_transform(outx, channel); }

            if (usePhasor) { processPhasor(args, channel); }
            else {
                processTrigger(args, channel);
            }
            // writeVoltages(channel);
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
    bool checkReset()
    {
        const bool resetConnected = inputs[INPUT_RST].isConnected();
        if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            resetPulse.trigger(1e-3F);  // Start ignoring clock pulses for 1ms
            for (int i = 0; i < NUM_CHANNELS; ++i) {
                clockTracker[i].init(keepPeriod ? clockTracker[i].getPeriod() : 0.F);
                trigOutPulses[i].reset();
                trigOutXPulses[i].reset();
                prevGateIndex[i] = rex.getStart(i, MAX_GATES);

                if (getGate(rex.getStart(i))) {
                    trigOutPulses[i].trigger(
                        params[PARAM_DURATION].getValue() * 0.1F *
                        inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, i) *
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
                lights[LIGHTS_GATE + i].setBrightness(getBitGate(i));
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
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::INPUT_RST));

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