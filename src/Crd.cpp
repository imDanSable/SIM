
#include <array>
#include "GaitX.hpp"
#include "InX.hpp"
#include "ModX.hpp"
#include "OutX.hpp"
#include "PhasorAnalyzers.hpp"
#include "ReX.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "glide.hpp"
#include "plugin.hpp"
#include "wrappers.hpp"

using constants::NUM_CHANNELS;

class Crd : public biexpand::Expandable<float> {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUT_NEXT, INPUT_RST, INPUTS_LEN };
    enum OutputId { TRIG_OUTPUT, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 32), LIGHTS_LEN };

   private:
    friend struct CrdWidget;

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;
    ModXAdapter modx;
    GaitXAdapter gaitx;

    /// @brief: is the current step modified?
    /// XXX this should be an array so that each channels have its modParams, or see if we can use
    /// direct getters
    ModXAdapter::ModParams modParams{};
    std::array<int, 16> randomizedSteps{};

    bool usePhasor = false;
    float gateLength = 1e-3F;

    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger nextTrigger;
    dsp::PulseGenerator resetPulse;  // ignore clock for 1ms after reset

    std::array<HCVPhasorStepDetector, MAX_GATES> stepDetectors;
    std::array<HCVPhasorSlopeDetector, MAX_GATES> slopeDetectors;
    std::array<HCVPhasorGateDetector, MAX_GATES> gateDetectors;

    std::array<HCVPhasorGateDetector, MAX_GATES> subGateDetectors;
    std::array<HCVPhasorStepDetector, MAX_GATES> subStepDetectors;

    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    //@brief: Pulse generators for trig out
    // std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutPulses = {};
    std::array<dsp::PulseGenerator, NUM_CHANNELS> eocTrigger = {};

    //@brief: Adjusts phase per channel so 10V doesn't revert back to zero.
    bool connectEnds = false;
    bool keepPeriod = false;

    std::array<dsp::TTimer<float>, NUM_CHANNELS> nextTimer = {};

    std::array<glide::GlideParams, NUM_CHANNELS> glides;

    dsp::ClockDivider uiDivider;

    bool readVoltages(bool forced = false)
    {
        const bool changed = this->cacheState.needsRefreshing();
        if (changed || forced) {
            // nothing to read
            cacheState.refresh();
        }
        return changed;
    }

    void writeVoltages()
    {
        const auto channels = (!inputs[INPUT_DRIVER].isConnected()) ? 0 : writeBuffer().size();
        outputs[OUTPUT_CV].setChannels(channels);
        if (!channels) {
            outputs[OUTPUT_CV].setVoltage(0.F);
            return;
        }
        outputs[OUTPUT_CV].writeVoltages(writeBuffer().data());
    }

   public:
    Crd()
        : biexpand::Expandable<float>({{modelReX, &rex}, {modelInX, &inx}},
                                      { {modelGaitX, &gaitx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(INPUT_DRIVER, "phasor or clock");
        configInput(INPUT_NEXT, "Trigger to advance to the next step");
        configInput(INPUT_RST, "Reset");
        configOutput(TRIG_OUTPUT, "Step trigger");
        configOutput(OUTPUT_CV, "Main");
        configCache({INPUT_DRIVER, INPUT_NEXT, INPUT_RST});
        for (int i = 0; i < NUM_CHANNELS; i++) {
            lights[LIGHT_STEP + i].setBrightness(0.02F);
        }
        for (int i = 0; i < MAX_GATES; i++) {
            subGateDetectors[i].setSmartMode(true);
            subGateDetectors[i].setGateWidth(1e-3F);
        }
        uiDivider.setDivision(constants::UI_UPDATE_DIVIDER);
    }

    void onReset() override
    {
        usePhasor = false;
        connectEnds = false;
        clockTracker.fill({});
    }
    void updateProgressLights(int numChannels)
    {
        // if (readBuffer().empty()) { return; }
        for (int lightIdx = 0; lightIdx < MAX_STEPS; ++lightIdx) {
            bool lightOn = false;
            for (int chan = 0; chan < numChannels; ++chan) {
                const int start = rex.getStart(chan);
                if (((stepDetectors[chan].getCurrentStep()) %
                         static_cast<int>(readBuffer().size()) +
                     start) %
                        MAX_STEPS ==
                    lightIdx) {
                    lightOn = true;
                    break;
                }
            }
            lights[LIGHT_STEP + lightIdx].setBrightness(lightOn ? 1.F : 0.02F);
        }
    }
    inline static float getNotePhaseFraction(float curPhase, int steps)
    {
        return fmodf(curPhase * steps, 1.F);
    }
    inline void processNotePhaseOutput(float notePhase, int channel)
    {
        gaitx.setPhi(notePhase * 10.F, channel);
    }

    void processAuxOutputs(const ProcessArgs& args,
                           int channel,
                           int numSteps,
                           int curStep,
                           float curPhase,
                           bool eoc)
    {
        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        if (gaitx.getStepConnected()) { gaitx.setStep(curStep, numSteps, channel); }
        float notePhase{};
        // if (gaitx.getCrdConnected() || (trigOutConnected)) {
        notePhase = getNotePhaseFraction(curPhase, numSteps);
        processNotePhaseOutput(notePhase, channel);
        // }
        if (gaitx.getEOCConnected()) {
            gaitx.setEOC(eocTrigger[channel].process(args.sampleTime) * 10.F, channel);
            if (eoc) { eocTrigger[channel].trigger(1e-3F); }
        }

        if (trigOutConnected) {
            bool high = false;
            if (modParams.reps > 1) {
                subStepDetectors[channel].setNumberSteps(modParams.reps);
                subStepDetectors[channel].setMaxSteps(MAX_STEPS);
                subStepDetectors[channel](notePhase);
                const int curSubStep = subStepDetectors[channel].getCurrentStep();

                const float subFraction = fmodf(notePhase * modParams.reps, 1.F);
                subGateDetectors[channel].setGateWidth(modx.getRepDur());
                subGateDetectors[channel].setSmartMode(true);
                const bool subStepGateTrigger =
                    (curSubStep < modParams.reps) && subGateDetectors[channel](subFraction);
                high = subStepGateTrigger;
            }
            gateDetectors[channel].setGateWidth(gateLength);
            gateDetectors[channel].setSmartMode(true);
            const bool gateTrigger = gateDetectors[channel](notePhase);
            outputs[TRIG_OUTPUT].setVoltage(10.F * (high || gateTrigger), channel);
        }
    }
    void updateModParams(int curStep)
    {
        if (modx) { modParams = modx.getParams(curStep); }
        if (modParams.prob < 1.0F) {
            if (random::uniform() > modParams.prob) {
                randomizedSteps[curStep] = random::u32() % readBuffer().size();
            }
        }
        else {
            randomizedSteps[curStep] = curStep;
        }
    }

    /// @brief Uses clock to calculate the phase in when triggers to advance
    /// @return the phase within the sequence
    /// 99% duplicate of Spike
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
        const int numSteps = inx ? inx.getLastConnectedInputIndex() + 1 : 0;
        // update our next trigger normalled to the clock if not connected
        const bool isNextTriggered =
            (isNextConnected
                 ? nextTrigger.process(inputs[INPUT_NEXT].getNormalVoltage(0.F, channel))
                 : isClockTriggered) &&
            isClockConnected;
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

    void processInxIn(const ProcessArgs& args)
    {
        if (!inx) { return; }

        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        const int numSteps = inx.getLastConnectedInputIndex() + 1;

        if (numSteps == 0) { return; }
        // for (int channel = 0; channel < channels; ++channel) {
        {
            int channel = 0;
            float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);

            stepDetectors[channel].setNumberSteps(numSteps);
            stepDetectors[channel].setMaxSteps(PORT_MAX_CHANNELS);
            if (connectEnds) { curCv = clamp(curCv, .0f, 9.9999f); }
            float normalizedPhasor =
                !usePhasor ? timeToPhase(args, channel, curCv) : wrappers::wrap(0.1F * curCv);
            const bool newStep = stepDetectors[channel](normalizedPhasor);
            const bool eoc = stepDetectors[channel].getEndOfCycle();
            const int curStep = (stepDetectors[channel].getCurrentStep());
            assert(curStep >= 0 && curStep < numSteps);  // NOLINT
            const float fractionalIndex = stepDetectors[channel].getFractionalStep();
            bool reversePhasor = slopeDetectors[channel](normalizedPhasor) < 0.0f;

            if (cvOutConnected) {
                const int numChannels = inx.getChannels(curStep);
                writeBuffer().resize(numChannels);
                // std::copy_n(iters::PortVoltageIterator(inx.getVoltages(prevStepIndex[channel])),
                std::copy_n(iters::PortVoltageIterator(inx.getVoltages(curStep)), numChannels,
                            writeBuffer().begin());
            }
            processAuxOutputs(args, channel, numSteps, curStep, curCv, eoc);
        }
    }

    /// 90% dupclicate of Spike
    void checkReset()
    {
        const bool resetConnected = inputs[INPUT_RST].isConnected();
        if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            resetPulse.trigger(1e-3F);  // ignore clock for 1ms after reset
            for (int i = 0; i < NUM_CHANNELS; ++i) {
                clockTracker[i].init(keepPeriod ? clockTracker[i].getPeriod() : 0.1F);
                nextTrigger.reset();
                stepDetectors[i].setStep(0);
                nextTimer[i].reset();
            }
        }
    }

    void process(const ProcessArgs& args) override
    {
        const bool driverConnected = inputs[INPUT_DRIVER].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        if (!driverConnected && !cvOutConnected) { return; }
        // const auto inputChannels = inputs[INPUT_DRIVER].getChannels();
        // XXX Here disable polyphony for the input clock for now
        const auto inputChannels = inx.getLastConnectedInputIndex() + 1;


        if (!usePhasor) { checkReset(); }
        // processPolyIn(args, inputChannels);
        processInxIn(args);
        gaitx.setChannels(1);
        if (trigOutConnected) { outputs[TRIG_OUTPUT].setChannels(inputChannels); }

        if (uiDivider.process()) { updateProgressLights(1); }

        writeVoltages();

        if (gaitx && gaitx->cacheState.isDirty()) { gaitx->cacheState.refresh(); }
        // if (outx && outx->cacheState.isDirty()) { outx->cacheState.refresh(); } // we're not outx
        // compatible
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
        json_object_set_new(rootJ, "keepPeriod", json_boolean(keepPeriod));
        json_object_set_new(rootJ, "gateLength", json_real(gateLength));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ) { connectEnds = json_is_true(connectEndsJ); }
        json_t* keepPeriodJ = json_object_get(rootJ, "keepPeriod");
        if (keepPeriodJ) { keepPeriod = json_is_true(keepPeriodJ); }
        json_t* gateLengthJ = json_object_get(rootJ, "gateLength");
        if (gateLengthJ) { gateLength = json_real_value(gateLengthJ); }
    }

   private:
    void onUpdateExpanders(bool isRight) override
    {
        // performTransforms(true);
        if (!modx) { modParams = {}; }
    }
};

using namespace dimensions;  // NOLINT

struct CrdWidget : public SIMWidget {
   public:
    struct GateLengthSlider : ui::Slider {
        GateLengthSlider(float* gateLenghtSrc, float minDb, float maxDb)
        {
            // quantity = new SliderQuantity<float>(gateLenghtSrc, minDb, maxDb, 1e-3F, "Gate
            // Length",
            quantity = new SliderQuantity<float>(gateLenghtSrc, minDb, maxDb, .1e-3F, "Gate Length",
                                                 "step duration", 3);
        }
        ~GateLengthSlider() override
        {
            delete quantity;
        }
    };

    explicit CrdWidget(Crd* module)
    {
        constexpr float centre = 1.5 * HP;
        setModule(module);
        setSIMPanel("Crd");

        float ypos{};
        ypos = JACKYSTART - JACKNTXT;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Crd::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Crd::INPUT_NEXT));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Crd::INPUT_RST));

        float y = ypos + JACKYSPACE;
        float dy = 2.4F;
        for (int i = 0; i < 8; i++) {
            auto* lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((HP), y + i * dy)), module, Crd::LIGHT_STEP + (i));
            addChild(lli);

            lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((2 * HP), y + i * dy)), module, Crd::LIGHT_STEP + (8 + i));
            addChild(lli);
        }

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, ypos += 3 * JACKNTXT)), module,
                                                Crd::TRIG_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                                Crd::OUTPUT_CV));

        if (!module) { return; }
        module->connectionLights.addDefaultConnectionLights(this, Crd::LIGHT_LEFT_CONNECTED,
                                                            Crd::LIGHT_RIGHT_CONNECTED);
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Crd*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));

        menu->addChild(new MenuSeparator);  // NOLINT

#ifndef NOPHASOR
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
#endif
        menu->addChild(
            createBoolPtrMenuItem("Remember speed after reset", "", &module->keepPeriod));

        auto* gateLengthSlider = new GateLengthSlider(&(module->gateLength), 1e-3F, 1.F);
        gateLengthSlider->box.size.x = 200.0f;
        menu->addChild(gateLengthSlider);
    }
};

Model* modelCrd = createModel<Crd, CrdWidget>("Crd");  // NOLINT