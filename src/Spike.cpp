// TODO: Colored lights
// BUG: Cut no longer works
// SOMEDAYMAYBE: Considder using booleantriggers (like hvc)
// SOMEDAYMAYBE: Menu option Zero output when no phasor movement

#include <array>
#include <cassert>
#include <cmath>
#include <vector>
#include "DebugX.hpp"
#include "GaitX.hpp"
#include "InX.hpp"
#include "ModX.hpp"
#include "OutX.hpp"
#include "ReX.hpp"
#include "Segment.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

#include "DSP/Phasors/HCVPhasorAnalyzers.h"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;
using iters::ParamIterator;

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
    /// XXX this should be an array so that each channels have its modParams?
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

    /// @brief: Address used by Segment2x8 to paint current step
    int activeIndex = 0;

    // std::array<std::bitset<MAX_GATES>, MAX_BANKS> bitMemory = {};
    std::array<bool, MAX_GATES> bitMemory = {};
    std::array<bool, MAX_GATES> randomizedMemory = {};

    dsp::Timer uiTimer;

    /// @brief: returns the normalized relative gate duration of step
    float getDuration(int step) const
    {
        return params[PARAM_DURATION].value * 0.1F *
               const_cast<Input*>(&inputs[INPUT_DURATION_CV])  // NOLINT
                   ->getNormalPolyVoltage(10.F, step);         // NOLINT
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
        for (int i = 0; i < MAX_GATES; i++) {
            subGateDetectors[i].setSmartMode(true);
        }
        configDirtyFlags();
    }

    void onReset() override
    {
        bitMemory.fill(false);
        connectEnds = false;
        channelProperties = {};
    }

    /// @returns: gateOn, triggered, step, fractional step, reverse direction
    std::tuple<bool, bool, int, float, bool> checkPhaseGate(int channel, float normalizedPhasor)
    {
        const float numSteps = readBuffer().size();
        stepDetectors[channel].setNumberSteps(numSteps);
        stepDetectors[channel].setMaxSteps(PORT_MAX_CHANNELS);

        bool triggered{};
        triggered = stepDetectors[channel](normalizedPhasor);
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

    bool applyModParams(int channel, int step, bool gateOn, bool newStep, float fraction)
    {
        if (!modx) { return gateOn; }
        if (newStep) {  // We need to process triggered, even when gateOn is false
            if (modx) { modParams = modx.getParams(step); }

            if (modParams.prob < 1.0F) {
                randomizedMemory[step] = random::uniform() < modParams.prob;
            }

            if (modParams.reps > 1) {
                // float duration = std::max(getDuration(step) * modx.getRepDur(), 1e-3F);
                float duration = std::max(modx.getRepDur(), 1e-3F);
                subGateDetectors[channel].setGateWidth(duration);
                /* bool triggered = */ subGateDetectors[channel](
                    fraction);  // Should trigger per definition
            }
        }
        if (gateOn) {
            if (modParams.prob < 1.0F) {
                gateOn = randomizedMemory[step];
                if (!gateOn) { return false; }
            }
        }

        if (readBuffer()[step]) {  // Process even when gateOn is false, we might be in the
                                   // gateOff part of a gateOn
            if (modParams.glide) { return true; }
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
        bool outxGateOn{};
        if (outx) {
            // Pre cut: calculate if the gate is on given no cuts have taken place for outx
            // TEST: Will this be ok for glide/reps and normalled ports on outx?
            std::tie(outxGateOn, std::ignore, std::ignore, std::ignore, std::ignore) =
                (checkPhaseGate(channel, normalizedPhasor));
            // Cut: perform the cut
            transform(outx);
        }
        // Post cut: calculate if the gate is on given the cuts have taken place for main out
        bool gateOn{};
        bool newStep{};
        float fraction{};
        int step{};
        bool reverse{};
        std::tie(gateOn, newStep, step, fraction, reverse) =
            checkPhaseGate(channel, normalizedPhasor);
        bool final = applyModParams(channel, step, gateOn, newStep, fraction);
        // Write main out (change to buffer when enabling polyphonic drivers)
        outputs[OUTPUT_GATE].setVoltage(final ? 10.F : 0.0f, channel);
        const int currentIndex = (stepDetectors[channel].getCurrentStep());
        // Write outx
        outx.setPortGate(currentIndex, outxGateOn, channel);
        // Write gaitx
        gaitx.setPhi(fraction * 10.F, channel);
        gaitx.setStep(step);
        gaitx.setEOC(stepDetectors[channel].getEndOfCycle() * 10.F, channel);
        // Set activeIndex for segment
        activeIndex = (currentIndex + rex.getStart()) % MAX_GATES;
    }

    void performTransforms(bool forced = false)  // 100% same as Bank
    {
        bool changed = readVoltages(forced);
        bool dirtyAdapters = false;
        if (!changed && !forced) { dirtyAdapters = this->dirtyAdapters(); }
        // Update our buffer because an adapter is dirty and our buffer needs to be updated
        if (!changed && !forced && dirtyAdapters) { readVoltages(true); }

        if (changed || dirtyAdapters || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                transform(*adapter);
            }
            // Skip outx, we will do that later
            // if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
            // transform(outx);
            // writeVoltages();
        }
    }

    bool readVoltages(bool forced = false)
    {
        const bool changed = this->isDirty();
        if (changed || forced) {
            //Assign the first 16 params to readBuffer
            readBuffer().resize(16);
            std::copy_n(ParamIterator{params.begin()}, 16, readBuffer().begin());

            // readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
            refresh();
        }
        return changed;

        // readBuffer().resize(MAX_GATES);
        // std::copy(bitMemory.begin(), bitMemory.end(), readBuffer().begin());

        // readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
        // DEBUG("readBuffer size: %d", readBuffer().size());
    }
    void writeVoltages(int channel)
    {
        // This should be  something like:
        // outputs[OUTPUT_GATE].writeVoltages(writeBuffer().data());
        // outputs[OUTPUT_GATE].setVoltage(writeBuffer()[channel] ? 10.F : 0.F, channel);
    }

    /// @brief Uses clock to calculate the phase in when triggers to advance
    /// @return the phase within the sequence
    /// 100% dupclicate of Phi
    float timeToPhase(const ProcessArgs& args, int channel, float cv)
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
        // readVoltages();
        // performTransforms();
        if (!usePhasor) { checkReset(); }
        const int numSteps = readBuffer().size();
        if (numSteps == 0) {
            writeBuffer().resize(0);
            return;
        }
        for (int channel = 0; channel < numChannels; channel++) {
            outputs[OUTPUT_GATE].setChannels(numChannels);
            const float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);
            performTransforms();
            // for (biexpand::Adapter* adapter : getLeftAdapters()) {
            //     transform(*adapter);
            // }
            // Don't transform outx, we will do that later
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
    // 90% double code with Phi
    void checkReset()
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
struct SpikeWidget : public SIMWidget {
    explicit SpikeWidget(Spike* module)
    {
        setModule(module);
        setSIMPanel("Spike");
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
                    mm2px(Vec(HP + (i * 2 * HP), JACKYSTART + (j)*JACKYSPACE)), module,
                    Spike::PARAM_GATE + (j + i * 8), Spike::LIGHTS_GATE + (j + i * 8)));
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
        assert(module);
        SIMWidget::appendContextMenu(menu);
        menu->addChild(new MenuSeparator);
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
        menu->addChild(createBoolPtrMenuItem("Keep Period after Reset", "", &module->keepPeriod));
    }
};

Model* modelSpike = createModel<Spike, SpikeWidget>("Spike");  // NOLINT