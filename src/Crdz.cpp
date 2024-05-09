
#include <array>
#include <cmath>
#include <span>
#include "GaitX.hpp"
#include "ReX.hpp"
#include "Shared.hpp"
#include "comp/knobs.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"
#include "plugin.hpp"
#include "sp/PhasorAnalyzers.hpp"
#include "sp/glide.hpp"

using namespace constants;  // NOLINT

constexpr int NUM_CRDZ = 16;

class Crdz : public biexpand::Expandable<float> {
   public:
    enum ParamId { PARAM_COMPENSATE, PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUT_RST, ENUMS(INPUT_CRD, NUM_CRDZ), INPUTS_LEN };
    enum OutputId { OUTPUT_CORR, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 16), LIGHTS_LEN };

   private:
    friend struct CrdzWidget;

    RexAdapter rex;
    GaitXAdapter gaitx;

    // int rexStart = 0;
    // int rexLen = NUM_CRDZ;
    float prevCompensation = 0.F;
    std::array<float, NUM_CHANNELS> corrections = {};
    int numSteps = 0;
    int curStep = 0;
    bool usePhasor = false;
    bool allowReverseTrigger = false;
    bool polyLdnssOut = false;

    rack::dsp::SchmittTrigger resetTrigger;
    rack::dsp::SchmittTrigger nextTrigger;
    rack::dsp::SchmittTrigger prevTrigger;
    dsp::PulseGenerator resetPulse;  // ignore clock for 1ms after reset
    sp::HCVPhasorStepDetector stepDetector;
    rack::dsp::PulseGenerator eocTrigger = {};
    rack::dsp::PulseGenerator newStepTrigger = {};
    bool disconnectEnds = false;
    std::array<dsp::TTimer<float>, NUM_CHANNELS> nextTimer = {};
    dsp::ClockDivider uiDivider;
    std::span<Input> chords;
    std::span<Light> indicatorLights;

   public:
    Crdz()
        : biexpand::Expandable<float>({/* {modelReX, &rex} , {modelInX, &inx}*/},
                                      {/*{modelGaitX, &gaitx}*/})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PARAM_COMPENSATE, 0.F, 1.F, 0.4F, "Compensation amount", "%", 0.F, 100.F);
        configInput(INPUT_DRIVER, "trigger or phasor input to advance or address step");
        configInput(INPUT_RST, "Reset");
        configOutput(OUTPUT_CORR, "Volume correction");
        // configOutput(OUTPUT_TRIG, "Step trigger");
        // configOutput(OUTPUT_EOC, "End of cycle");
        configOutput(OUTPUT_CV, "Main");
        configCache({INPUT_DRIVER, INPUT_RST}, {});
        for (int i = 0; i < NUM_CHANNELS; i++) {
            lights[LIGHT_STEP + i].setBrightness(0.02F);
        }
        uiDivider.setDivision(constants::UI_UPDATE_DIVIDER);
        indicatorLights = {lights.data() + LIGHT_STEP, 16};  // NOLINT
        chords = {inputs.data() + INPUT_CRD, NUM_CRDZ};      // NOLINT
    }

    void onReset() override
    {
        usePhasor = false;
        disconnectEnds = false;
        curStep = 0;
    }

    void updateProgressLights()
    {
        const bool driverConnected = inputs[INPUT_DRIVER].isConnected();
        if ((numSteps == 0) || !driverConnected) {
            for (auto& light : indicatorLights) {
                light.setBrightness(0.02F);
            }
            return;
        }
        for (int lightIdx = 0; lightIdx < MAX_STEPS; ++lightIdx) {
            bool lightOn = false;
            if (((curStep) % numSteps) % MAX_STEPS == lightIdx) { lightOn = true; }
            lights[LIGHT_STEP + lightIdx].setBrightness(lightOn ? 1.F : 0.02F);
        }
    }

    void processAuxOutputs(const ProcessArgs& args,
                           float curPhase,
                           bool eoc,
                           int numChannels,
                           bool newStep)
    {
        // if (outputs[OUTPUT_TRIG].isConnected()) {
        //     if (newStep) {
        //         if (outputs[OUTPUT_TRIG].isConnected()) { newStepTrigger.trigger(gateLength); }
        //     }
        //     const bool high = newStepTrigger.process(args.sampleTime);
        //     outputs[OUTPUT_TRIG].setChannels(polyStepTrigger ? numChannels : 1);
        //     for (int i = 0; i < (polyStepTrigger ? numChannels : 1); i++) {
        //         outputs[OUTPUT_TRIG].setVoltage(high * 10.F, i);
        //     }
        // }
        if (outputs[OUTPUT_CORR].isConnected()) {
            outputs[OUTPUT_CORR].setChannels(polyLdnssOut ? numChannels : 1);
            for (int i = 0; i < (polyLdnssOut ? numChannels : 1); i++) {
                outputs[OUTPUT_CORR].setVoltage(getVolumeCorrection(numChannels) * 10.F, i);
            }
        }
        // if (outputs[OUTPUT_EOC].isConnected()) {
        //     outputs[OUTPUT_EOC].setChannels(polyEOC ? numChannels : 1);
        //     if (eoc) { eocTrigger.trigger(1e-3F); }
        //     const bool high = eocTrigger.process(args.sampleTime);
        //     for (int i = 0; i < (polyEOC ? numChannels : 1); i++) {
        //         outputs[OUTPUT_EOC].setVoltage(high * 10.F, i);
        //     }
        // }
    }

    void processInxIn(const ProcessArgs& args)
    {
        const bool ignoreClock = resetPulse.process(args.sampleTime);

        float driverCv = inputs[INPUT_DRIVER].getNormalVoltage(0.F);

        float phase = 0.F;
        bool newStep = false;
        bool eoc = false;
        // int rexStartDiff = rexStart - rex.getStart();
        // const int start = rex.getStart();
        numSteps = std::min(getLastConnectedInputIndex() + 1, rex.getLength());
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();

        if (numSteps == 0) { return; }

        if (!usePhasor) {
            const bool isNextTriggered = nextTrigger.process(inputs[INPUT_DRIVER].getVoltage());
            const bool isPrevTriggered =
                !allowReverseTrigger || isNextTriggered
                    ? false
                    : prevTrigger.process(-inputs[INPUT_DRIVER].getVoltage());
            newStep = (isNextTriggered || isPrevTriggered) && !ignoreClock;

            if (newStep) {
                curStep = eucMod(curStep + isNextTriggered - isPrevTriggered, numSteps);
                eoc = curStep == numSteps - 1;
            }
            // if (rexStartDiff != 0) {
            //     curStep = eucMod(curStep + eucMod(rexStartDiff, numSteps), numSteps);
            //     rexStart = rex.getStart();
            // }
        }
        else {
            stepDetector.setNumberSteps(numSteps);
            stepDetector.setMaxSteps(PORT_MAX_CHANNELS);
            if (disconnectEnds) { phase = clamp(limit(driverCv / 10.F, 1.F), 0.F, 1.F); }
            else {
                phase = wrap(clamp(driverCv / 10.F, 0.F, 1.F), 0.F, 1.F);
            }
            newStep = stepDetector(phase);
            eoc = stepDetector.getEndOfCycle();
            curStep = (stepDetector.getCurrentStep());
        }
        assert(curStep >= 0 && curStep < numSteps);  // NOLINT
        const int numChannels = chords[curStep].getChannels();
        if (cvOutConnected && newStep) {
            outputs[OUTPUT_CV].setChannels(numChannels);
            outputs[OUTPUT_CV].writeVoltages(chords[curStep].getVoltages());
        }
        processAuxOutputs(args, phase, eoc, numChannels, newStep);
    }

    void checkReset()
    {
        const bool resetConnected = inputs[INPUT_RST].isConnected();
        if (resetConnected && resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            resetPulse.trigger(1e-3F);  // ignore clock for 1ms after reset
            curStep = 0;
        }
    }
    void process(const ProcessArgs& args) override
    {
        if (uiDivider.process()) { updateProgressLights(); }
        const bool driverConnected = inputs[INPUT_DRIVER].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        if (!driverConnected && !cvOutConnected) { return; }
        if (!usePhasor) { checkReset(); }
        processInxIn(args);
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_boolean(disconnectEnds));
        json_object_set_new(rootJ, "allowReverseTrigger", json_boolean(allowReverseTrigger));
        json_object_set_new(rootJ, "polyLdnssOut", json_boolean(polyLdnssOut));
        // json_object_set_new(rootJ, "polyStepTrigger", json_boolean(polyStepTrigger));
        // json_object_set_new(rootJ, "polyEOC", json_boolean(polyEOC));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ) { disconnectEnds = json_is_true(connectEndsJ); }
        json_t* allowReverseTriggerJ = json_object_get(rootJ, "allowReverseTrigger");
        if (allowReverseTriggerJ) { allowReverseTrigger = json_is_true(allowReverseTriggerJ); }
        json_t* polyLdnssOutJ = json_object_get(rootJ, "polyLdnssOut");
        if (polyLdnssOutJ) { polyLdnssOut = json_is_true(polyLdnssOutJ); }
        // json_t* polyStepTriggerJ = json_object_get(rootJ, "polyStepTrigger");
        // if (polyStepTriggerJ) { polyStepTrigger = json_is_true(polyStepTriggerJ); }
        // json_t* polyEOCJ = json_object_get(rootJ, "polyEOC");
        // if (polyEOCJ) { polyEOC = json_is_true(polyEOCJ); }
    }

   private:
    int getLastConnectedInputIndex() const
    {
        for (int i = chords.size() - 1; i >= 0; i--) {
            if (chords[i].isConnected()) { return i; }
        }
        return -1;  // Return -1 if no connected element is found
    }
    float getVolumeCorrection(int channelCount)
    {
        const float& currentCompensation = params[PARAM_COMPENSATE].getValue();
        if (prevCompensation != currentCompensation) {
            prevCompensation = currentCompensation;
            // Calculate the correction factor for each possible channelCount
            for (int i = 0; i < NUM_CHANNELS; i++) {
                corrections[i] = 1.0F / std::pow(i, currentCompensation);
            }
        }
        return corrections[channelCount];
    }
};

using namespace dimensions;  // NOLINT

struct CrdzWidget : public SIMWidget {
   public:
    explicit CrdzWidget(Crdz* module)
    {
        constexpr float right = 3 * HP;
        constexpr float left = HP;
        setModule(module);
        setSIMPanel("Crdz");

        float ypos = JACKYSTART - JACKNTXT;
        addInput(
            createInputCentered<comp::SIMPort>(mm2px(Vec(left, ypos)), module, Crdz::INPUT_DRIVER));

        addInput(
            createInputCentered<comp::SIMPort>(mm2px(Vec(right, ypos)), module, Crdz::INPUT_RST));

        for (int i = 0; i < 8; i++) {
            addInput(createInputCentered<comp::SIMPort>(
                mm2px(Vec(left, JACKYSTART + (i)*JACKYSPACE)), module, Crdz::INPUT_CRD + i));

            addInput(createInputCentered<comp::SIMPort>(
                mm2px(Vec(right, JACKYSTART + (i)*JACKYSPACE)), module, Crdz::INPUT_CRD + i + 8));

            addChild(createLightCentered<rack::TinySimpleLight<WhiteLight>>(
                mm2px(Vec(left + 4.2F, JACKYSTART + (i)*JACKYSPACE)), module,
                Crdz::LIGHT_STEP + i));
            addChild(createLightCentered<rack::TinySimpleLight<WhiteLight>>(
                mm2px(Vec(right - 4.2F, JACKYSTART + (i)*JACKYSPACE)), module,
                Crdz::LIGHT_STEP + i + 8));
        }

        // addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)),
        // module,
        //                                               Crdz::OUTPUT_TRIG));

        // addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)),
        // module,
        //                                               Crdz::OUTPUT_EOC));
        addOutput(createOutputCentered<comp::SIMPort>(
            mm2px(Vec(left, ypos = LOW_ROW + JACKYSPACE - 9.F)), module, Crdz::OUTPUT_CORR));

        addParam(createParamCentered<comp::SIMSmallKnob>(mm2px(Vec(left, ypos += JACKYSPACE)),
                                                         module, Crdz::PARAM_COMPENSATE));

        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, LOW_ROW + JACKYSPACE - 9.F)),
                                                      module, Crdz::OUTPUT_CV));

        if (!module) { return; }

        // module->connectionLights.addDefaultConnectionLights(this, Crdz::LIGHT_LEFT_CONNECTED,
        // -1);
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Crdz*>(this->module);
        assert(module);  // NOLINT

        // SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));

        menu->addChild(new MenuSeparator);  // NOLINT

        // #ifndef NOPHASOR
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(
            createBoolPtrMenuItem("Disconnect Begin and End", "", &module->disconnectEnds));
        // #endif
        menu->addChild(createBoolPtrMenuItem("Negative 'clk' pulse steps in reverse direction", "",
                                             &module->allowReverseTrigger));
        menu->addChild(createBoolPtrMenuItem("Polyphonic Volume Correction output", "",
                                             &module->polyLdnssOut));
    }
};

Model* modelCrdz = createModel<Crdz, CrdzWidget>("Crdz");  // NOLINT