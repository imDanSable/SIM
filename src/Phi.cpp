// TODO: Trigout is still time based.
// TODO: use reversePhasor (for also needed for broken reverse glide and such)
// TODO: Bring InX mode up to date with the latest changes (modx, gaitx, etc)

#include <array>
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DebugX.hpp"
#include "GaitX.hpp"
#include "InX.hpp"
#include "ModX.hpp"
#include "OutX.hpp"
#include "ReX.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "glide.hpp"
#include "plugin.hpp"

using constants::NUM_CHANNELS;
class Phi : public biexpand::Expandable<float> {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_CV, INPUT_DRIVER, INPUT_NEXT, INPUT_RST, INPUTS_LEN };
    enum OutputId { TRIG_OUTPUT, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 32), LIGHTS_LEN };

    enum StepsSource { POLY_IN, INX };

   private:
    friend struct PhiWidget;

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;
    ModXAdapter modx;
    GaitXAdapter gaitx;
    DebugXAdapter debugx;

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
    std::array<HCVPhasorGateDetector, MAX_GATES> subGateDetectors;
    std::array<HCVPhasorStepDetector, MAX_GATES> subStepDetectors;

    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    //@brief: Pulse generators for trig out
    std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutPulses = {};
    std::array<dsp::PulseGenerator, NUM_CHANNELS> eocTrigger = {};

    //@brief: Adjusts phase per channel so 10V doesn't revert back to zero.
    bool connectEnds = false;
    bool keepPeriod = false;
    StepsSource stepsSource = POLY_IN;
    StepsSource preferedStepsSource = POLY_IN;

    std::array<dsp::TTimer<float>, NUM_CHANNELS> nextTimer = {};

    std::array<glide::GlideParams, NUM_CHANNELS> glides;

    enum StepOutputVoltageMode {
        SCALE_10_TO_16,
        SCALE_1_TO_10,  // DOCB_COMPATIBLE
        SCALE_10_TO_LENGTH
    };

    StepOutputVoltageMode stepOutputVoltageMode = StepOutputVoltageMode::SCALE_10_TO_16;

    bool readVoltages(bool forced = false)
    {
        if (stepsSource == POLY_IN) {
            const bool changed = this->isDirty();
            if (changed || forced) {
                auto& input = inputs[INPUT_CV];
                auto channels = input.isConnected() ? input.getChannels() : 0;
                readBuffer().resize(channels);
                if (channels > 0) {
                    std::copy_n(iters::PortVoltageIterator(input.getVoltages()), channels,
                                readBuffer().begin());
                }
                refresh();
            }
            return changed;
        }
        return false;
        // XXX Finish InX
    }

    void writeVoltages()
    {
        const auto channels =
            (!inputs[INPUT_CV].isConnected() || !inputs[INPUT_DRIVER].isConnected())
                ? 0
                : writeBuffer().size();
        outputs[OUTPUT_CV].setChannels(channels);
        if (!channels) {
            outputs[OUTPUT_CV].setVoltage(0.F);
            return;
        }
        outputs[OUTPUT_CV].writeVoltages(writeBuffer().data());
    }

   public:
    Phi()
        : biexpand::Expandable<float>(
              {{modelReX, &rex}, {modelInX, &inx}, {modelModX, &modx}},
              {{modelOutX, &outx}, {modelGaitX, &gaitx}, {modelDebugX, &debugx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(INPUT_CV, "Poly");
        configInput(INPUT_DRIVER, "phasor or clock pulse in");
        configInput(INPUT_NEXT, "Trigger to advance to the next step");
        configInput(INPUT_RST, "Reset");
        configOutput(TRIG_OUTPUT, "Step trigger");
        configOutput(OUTPUT_CV, "Main");
        for (int i = 0; i < NUM_CHANNELS; i++) {
            lights[LIGHT_STEP + i].setBrightness(0.02F);
        }
        for (int i = 0; i < MAX_GATES; i++) {
            subGateDetectors[i].setSmartMode(true);
            subGateDetectors[i].setGateWidth(1e-3F);
        }
        configDirtyFlags({INPUT_DRIVER});
    }

    void onReset() override
    {
        usePhasor = false;
        connectEnds = false;
        stepsSource = POLY_IN;
        preferedStepsSource = POLY_IN;
        clockTracker.fill({});
        trigOutPulses.fill({});
    }
    void updateProgressLights(int channels)
    {
        for (int step = 0; step < NUM_CHANNELS; ++step) {
            bool lightOn = false;
            for (int chan = 0; chan < channels; ++chan) {
                // if (prevStepIndex[chan] == step) {
                if (rex.getStart(chan) + stepDetectors[chan].getCurrentStep() == step) {
                    lightOn = true;
                    break;
                }
            }
            lights[LIGHT_STEP + step].setBrightness(lightOn ? 1.F : 0.02F);
        }
    }
    int getCurStep(float cv, int numSteps) const
    {
        const float delta = 0.00001F;  // to avoid jumps at 0 and 1
        const float curCv = clamp(cv / 10.F, delta * connectEnds, 1.F - delta * connectEnds);
        return numSteps > 0 ? static_cast<int>(curCv * numSteps) % numSteps : 0;
    }
    /// @brief: Returns the current substep, but does not check if substeps !+ 1
    static int getCurSubStep(float notePhase, int numSubSteps)
    {
        // XXX Do we need to connect ends here?
        // XXX Decide if we need to clamp numSubSteps a little bit because of floating point
        // inaccuracy
        return static_cast<int>(numSubSteps * notePhase) % numSubSteps;
    }
    void processStepOutput(int step, int channel, int totalSteps)
    {
        switch (stepOutputVoltageMode) {
            case SCALE_10_TO_16: gaitx.setStep(0.625 * step, channel); break;
            case SCALE_1_TO_10: gaitx.setStep(.1F * step, channel); break;
            case SCALE_10_TO_LENGTH:
                const float scaledStep = rack::rescale(step, 0.F, totalSteps - 1, 0.F, 10.F);
                gaitx.setStep(scaledStep, channel);
                break;
        }
    }

    float processNotePhaseOutput(float curPhase, int channel)
    {
        const float notePhase = fmodf(curPhase * readBuffer().size(), 1.F);
        gaitx.setPhi(notePhase * 10.F, channel);
        return notePhase;
    }

    void processAuxOutputs(const ProcessArgs& args,
                           int channel,
                           int numSteps,
                           int curStep,
                           float curPhase,
                           bool eoc)
    {
        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        if (gaitx.getStepConnected()) { processStepOutput(curStep, channel, numSteps); }
        float notePhase{};
        if (gaitx.getPhiConnected() || (trigOutConnected && modParams.reps > 1)) {
            notePhase = processNotePhaseOutput(curPhase, channel);
        }
        if (gaitx.getEOCConnected()) {
            gaitx.setEOC(eocTrigger[channel].process(args.sampleTime) * 10.F, channel);
            if (eoc) { eocTrigger[channel].trigger(1e-3F); }
        }

        if (trigOutConnected) {
            bool high = false;
            if (modParams.reps > 1) {
                // const int reps = modParams.reps;
                // const int curSubStep = static_cast<int>(notePhase * reps);
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
            bool ignoreClock = resetPulse.process(args.sampleTime);
            bool triggered =
                (high || trigOutPulses[channel].process(args.sampleTime)) && !ignoreClock;
            outputs[TRIG_OUTPUT].setVoltage(10 * triggered, channel);
        }
    }
    void processInxIn(const ProcessArgs& args, int channels)
    {
        // const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        // const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        // const int numSteps = inx.getLastConnectedInputIndex() + 1;
        // bool eoc = false;
        // if (numSteps == 0) { return; }
        // for (int channel = 0; channel < channels; ++channel) {
        //     const float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);
        //     int curStep{};
        //     if (usePhasor) {
        //         curStep = getCurStep(curCv, numSteps);
        //         if ((prevStepIndex[channel] != curStep)) {
        //             if (trigOutConnected) { trigOutPulses[channel].trigger(gateLength); }
        //             prevStepIndex[channel] = curStep;
        //         }
        //     }
        //     else {  /// XXX Is this doing triggers using time after we switched to phasor? but
        //     not
        //             /// yet inx
        //         const bool nextConnected = inputs[INPUT_NEXT].isConnected();
        //         const bool ignoreClock = resetPulse.process(args.sampleTime);
        //         const bool clockTrigger =
        //             clockTracker[channel].process(args.sampleTime, curCv) && !ignoreClock;  //
        //             NEXT
        //         const bool triggered =
        //             nextConnected
        //                 ? nextTrigger.process(inputs[INPUT_NEXT].getVoltage()) && !ignoreClock
        //                 : clockTrigger;
        //         // REFACTOR
        //         curStep = (prevStepIndex[channel] + triggered) % numSteps;
        //         if (triggered) {
        //             if (trigOutConnected) { trigOutPulses[channel].trigger(gateLength); }
        //             prevStepIndex[channel] = curStep;
        //         }
        //     }
        //     if (cvOutConnected) {
        //         const int numChannels = inx.getChannels(curStep);
        //         writeBuffer().resize(numChannels);
        //         std::copy_n(iters::PortVoltageIterator(inx.getVoltages(prevStepIndex[channel])),
        //                     numChannels, writeBuffer().begin());
        //     }
        //     processAuxOutputs(args, channel, numSteps, curStep, curCv, eoc);
        // }
    }
    void updateModParams(int curStep)
    {
        if (modx) { modParams = modx.getParams(curStep); }
        if (modParams.prob < 1.0F) {
            if (random::uniform() > modParams.prob) {
                randomizedSteps[curStep] = random::u32() % readBuffer().size();
            }
        }
    }

    /// @brief Uses clock to calculate the phase in when triggers to advance
    /// @return the phase within the sequence
    /// 100% duplicate of Spike
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
        debugx.setPort1(phase, channel);
        return phase;
    }

    void processPolyIn(const ProcessArgs& args, int channels)
    {
        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();

        const int numSteps = readBuffer().size();
        if (numSteps == 0) {
            writeBuffer().resize(0);
            return;
        }
        if (channels) {
            writeBuffer().resize(channels);  // XXX Isn't this done by the baseadapter class?
        }  // XXX I doubt this to be complete when we'll be using channels
        for (int channel = 0; channel < channels; ++channel) {
            const float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);
            stepDetectors[channel].setNumberSteps(numSteps);
            stepDetectors[channel].setMaxSteps(PORT_MAX_CHANNELS);
            const float normalizedPhasor =
                !usePhasor ? timeToPhase(args, channel, curCv) : scaleAndWrapPhasor(curCv);
            const bool newStep = stepDetectors[channel](normalizedPhasor);
            const bool eoc = stepDetectors[channel].getEndOfCycle();
            const int curStep = (stepDetectors[channel].getCurrentStep());
            debugx.setPort2(normalizedPhasor, channel);
            assert(curStep >= 0 && curStep < numSteps);  // NOLINT
            const float fractionalIndex = stepDetectors[channel].getFractionalStep();
            bool reversePhasor = slopeDetectors[channel](normalizedPhasor) < 0.0f;
            // Are we on a new step?
            if (newStep) {
                updateModParams(curStep);
                if (trigOutConnected) {
                    if (modParams.reps <= 1) {
                        trigOutPulses[channel].trigger(gateLength);
                    }  // Don't trigger if part of reps
                }
            }
            if (cvOutConnected || outx) {
                // Can the buffer size change? after updateModParams?
                // If it can, we'll crash here, or because of here.
                assert(randomizedSteps[curStep] < static_cast<int>(readBuffer().size()));  // NOLINT
                // Route through the random steps if prob < 1.0
                float cv = modParams.prob < 1.0F ? readBuffer().at(randomizedSteps[curStep])
                                                 : readBuffer().at(curStep);
                if (modParams.glide) {
                    if (newStep) {
                        // Initiate the glide
                        // XXX 303 does glide on NEXT step. Should we?
                        // For spike we just neeed to set the gate length to 100%
                        glides[channel].trigger(modParams.glideTime,
                                                outputs[OUTPUT_CV].getVoltage(channel), cv,
                                                modParams.glideShape);
                    }
                    cv = glides[channel].processPhase(fractionalIndex);
                }
                writeBuffer()[channel] = cv;
            }
            processAuxOutputs(args, channel, numSteps, curStep, normalizedPhasor, eoc);
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
                trigOutPulses[i].reset();
                stepDetectors[i].setStep(0);
                nextTimer[i].reset();
            }
        }
    }

    void performTransforms(bool forced = false)  // 100% same as Bank
    {
        bool changed = readVoltages(forced);
        bool dirtyAdapters = false;
        // XXX Dirty adapters not reporting dirty correctly
        if (!changed && !forced) { dirtyAdapters = this->dirtyAdapters(); }
        // Update our buffer because an adapter is dirty and our buffer needs to be updated
        if (!changed && !forced && dirtyAdapters) { readVoltages(true); }

        if (changed || dirtyAdapters || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                transform(*adapter);
            }
            if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
            transform(outx);
            writeVoltages();
        }
    }
    void process(const ProcessArgs& args) override
    {
        const bool driverConnected = inputs[INPUT_DRIVER].isConnected();
        const bool cvInConnected = inputs[INPUT_CV].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
        if (!driverConnected && !cvInConnected && !cvOutConnected) { return; }
        const auto inputChannels = inputs[INPUT_DRIVER].getChannels();
        performTransforms();

        if (!usePhasor) { checkReset(); }
        stepsSource == POLY_IN ? processPolyIn(args, inputChannels)
                               : processInxIn(args, inputChannels);
        gaitx.setChannels(inputChannels);
        if (trigOutConnected) { outputs[TRIG_OUTPUT].setChannels(inputChannels); }

        updateProgressLights(inputChannels);

        writeVoltages();
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
        json_object_set_new(rootJ, "keepPeriod", json_boolean(keepPeriod));
        json_object_set_new(rootJ, "stepsSource", json_integer(stepsSource));
        json_object_set_new(rootJ, "preferedStepsSource", json_integer(preferedStepsSource));
        json_object_set_new(rootJ, "stepOutputVoltageMode",
                            json_integer(static_cast<int>(stepOutputVoltageMode)));
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
        json_t* stepsSourceJ = json_object_get(rootJ, "stepsSource");
        if (stepsSourceJ != nullptr) {
            stepsSource = static_cast<StepsSource>(json_integer_value(stepsSourceJ));
            preferedStepsSource = stepsSource;
        };
        json_t* stepOutputVoltageModeJ = json_object_get(rootJ, "stepOutputVoltageMode");
        if (stepOutputVoltageModeJ) {
            stepOutputVoltageMode =
                static_cast<StepOutputVoltageMode>(json_integer_value(stepOutputVoltageModeJ));
        }
        json_t* gateLengthJ = json_object_get(rootJ, "gateLength");
        if (gateLengthJ) { gateLength = json_real_value(gateLengthJ); }
    }

   private:
    void onUpdateExpanders(bool isRight) override
    {
        if (!isRight) { updateStepsSource(); }
        performTransforms(true);
        // readVoltages();
    }

    void updateStepsSource()
    {
        // always possible to set to POLY_IN
        if (preferedStepsSource == POLY_IN) {
            stepsSource = POLY_IN;
            return;
        }
        // Take care of invalid state
        if (!inx && (stepsSource == INX)) { stepsSource = POLY_IN; }

        // check prefered sources
        if (preferedStepsSource == INX && inx) {
            stepsSource = INX;
            return;
        }
    }
};

using namespace dimensions;  // NOLINT

struct PhiWidget : public SIMWidget {
   public:
    struct GateLengthSlider : ui::Slider {
        GateLengthSlider(float* gateLenghtSrc, float minDb, float maxDb)
        {
            quantity = new SliderQuantity<float>(gateLenghtSrc, minDb, maxDb, 1e-3F, "Gate Length",
                                                 "s", 3);
        }
        ~GateLengthSlider() override
        {
            delete quantity;
        }
    };

    explicit PhiWidget(Phi* module)
    {
        constexpr float centre = 1.5 * HP;
        setModule(module);
        setSIMPanel("Phi");

        float ypos{};
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos = JACKYSTART - JACKNTXT)),
                                              module, Phi::INPUT_CV));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Phi::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Phi::INPUT_NEXT));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Phi::INPUT_RST));

        float y = ypos + JACKYSPACE;
        float dy = 2.4F;
        for (int i = 0; i < 8; i++) {
            auto* lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((HP), y + i * dy)), module, Phi::LIGHT_STEP + (i));
            addChild(lli);

            lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((2 * HP), y + i * dy)), module, Phi::LIGHT_STEP + (8 + i));
            addChild(lli);
        }

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, ypos += 3 * JACKNTXT)), module,
                                                Phi::TRIG_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                                Phi::OUTPUT_CV));

        if (!module) { return; }
        module->addDefaultConnectionLights(this, Phi::LIGHT_LEFT_CONNECTED,
                                           Phi::LIGHT_RIGHT_CONNECTED);
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Phi*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));

        menu->addChild(new MenuSeparator);  // NOLINT

        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
        menu->addChild(createBoolPtrMenuItem("Keep period", "", &module->keepPeriod));

        menu->addChild(createSubmenuItem(
            "Steps source",
            [=]() -> std::string {
                switch (module->stepsSource) {
                    case Phi::StepsSource::POLY_IN: return "Poly in";
                    case Phi::StepsSource::INX: return "InX";
                    default: return "";
                }
                return "";
            }(),
            [=](Menu* menu) {
                menu->addChild(createMenuItem(
                    "Poly in", module->stepsSource == Phi::POLY_IN ? "✔" : "", [=]() {
                        module->preferedStepsSource = Phi::StepsSource::POLY_IN;
                        module->updateStepsSource();
                    }));

                MenuItem* inx_item = createMenuItem(
                    "InX", module->stepsSource == Phi::StepsSource::INX ? "✔" : "", [=]() {
                        module->preferedStepsSource = Phi::StepsSource::INX;
                        module->updateStepsSource();
                    });
                if (!module->inx) { inx_item->disabled = true; }
                menu->addChild(inx_item);
            }));

        // Add a menu for the step output voltage modes
        menu->addChild(createIndexPtrSubmenuItem(
            "Step output voltage",
            {"0V..10V : First..16th", "0.1V per step", "0V..Length : First..Last"},
            &module->stepOutputVoltageMode));

        auto* gateLengthSlider = new GateLengthSlider(&(module->gateLength), 1e-3F, 1.F);
        gateLengthSlider->box.size.x = 200.0f;
        menu->addChild(gateLengthSlider);
    }
};

Model* modelPhi = createModel<Phi, PhiWidget>("Phi");  // NOLINT
