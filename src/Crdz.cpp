
#include <array>
#include <cmath>
#include <span>
#include "Shared.hpp"
#include "comp/knobs.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"
#include "plugin.hpp"
#include "sp/PhasorAnalyzers.hpp"
#include "sp/glide.hpp"

using namespace constants;  // NOLINT

constexpr int NUM_CRDZ = 8;

class Chrds : public Module {
   public:
    enum ParamId { PARAM_SHAPE, PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUT_RST, ENUMS(INPUT_CRD, NUM_CRDZ), INPUTS_LEN };
    enum OutputId { OUTPUT_TRIG, OUTPUT_LDNSS, OUTPUT_EOC, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 16), LIGHTS_LEN };

   private:
    friend struct CrdzWidget;

    int numSteps = 0;
    int curStep = 0;
    bool usePhasor = false;
    bool allowReverseTrigger = false;
    bool polyStepTrigger = false;
    bool polyLdnssOut = false;
    bool polyEOC = false;
    float gateLength = 1e-3F;

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
    Chrds()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PARAM_SHAPE, 0.F, 1.F, 0.4F, "Compensation amount", "%", 0.F,
                                  100.F);
        configInput(INPUT_DRIVER, "trigger or phasor input to advance or address step");
        configInput(INPUT_RST, "Reset");
        configOutput(OUTPUT_LDNSS, "Volume correction");
        configOutput(OUTPUT_TRIG, "Step trigger");
        configOutput(OUTPUT_EOC, "End of cycle");
        configOutput(OUTPUT_CV, "Main");
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
        if (outputs[OUTPUT_TRIG].isConnected()) {
            if (newStep) {
                if (outputs[OUTPUT_TRIG].isConnected()) { newStepTrigger.trigger(gateLength); }
            }
            const bool high = newStepTrigger.process(args.sampleTime);
            outputs[OUTPUT_TRIG].setChannels(polyStepTrigger ? numChannels : 1);
            for (int i = 0; i < (polyStepTrigger ? numChannels : 1); i++) {
                outputs[OUTPUT_TRIG].setVoltage(high * 10.F, i);
            }
        }
        if (outputs[OUTPUT_LDNSS].isConnected()) {
            outputs[OUTPUT_LDNSS].setChannels(polyLdnssOut ? numChannels : 1);
            for (int i = 0; i < (polyLdnssOut ? numChannels : 1); i++) {
                outputs[OUTPUT_LDNSS].setVoltage(getVolumeCorrection(numChannels) * 10.F, i);
            }
        }
        if (outputs[OUTPUT_EOC].isConnected()) {
            outputs[OUTPUT_EOC].setChannels(polyEOC ? numChannels : 1);
            if (eoc) { eocTrigger.trigger(1e-3F); }
            const bool high = eocTrigger.process(args.sampleTime);
            for (int i = 0; i < (polyEOC ? numChannels : 1); i++) {
                outputs[OUTPUT_EOC].setVoltage(high * 10.F, i);
            }
        }
    }

    void processInxIn(const ProcessArgs& args)
    {
        const bool ignoreClock = resetPulse.process(args.sampleTime);

        float driverCv = inputs[INPUT_DRIVER].getNormalVoltage(0.F);

        float phase = 0.F;
        bool newStep = false;
        bool eoc = false;
        numSteps = getLastConnectedInputIndex() + 1;
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
        json_object_set_new(rootJ, "polyStepTrigger", json_boolean(polyStepTrigger));
        json_object_set_new(rootJ, "polyLdnssOut", json_boolean(polyLdnssOut));
        json_object_set_new(rootJ, "polyEOC", json_boolean(polyEOC));
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
        json_t* polyStepTriggerJ = json_object_get(rootJ, "polyStepTrigger");
        if (polyStepTriggerJ) { polyStepTrigger = json_is_true(polyStepTriggerJ); }
        json_t* polyLdnssOutJ = json_object_get(rootJ, "polyLdnssOut");
        if (polyLdnssOutJ) { polyLdnssOut = json_is_true(polyLdnssOutJ); }
        json_t* polyEOCJ = json_object_get(rootJ, "polyEOC");
        if (polyEOCJ) { polyEOC = json_is_true(polyEOCJ); }
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
        float exponent = params[PARAM_SHAPE].getValue();
        return 1.0F / std::pow(channelCount, exponent);
    }
};

using namespace dimensions;  // NOLINT

struct CrdzWidget : public SIMWidget {
   public:
    explicit CrdzWidget(Chrds* module)
    {
        constexpr float right = 3.5 * HP;
        constexpr float left = 1.25 * HP;
        setModule(module);
        setSIMPanel("Crdz");

        for (int i = 0; i < NUM_CRDZ; i++) {
            addInput(createInputCentered<comp::SIMPort>(
                mm2px(Vec(left, JACKYSTART + (i)*JACKYSPACE)), module, Chrds::INPUT_CRD + i));
        }

        float ypos = JACKYSTART - JACKNTXT;
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(left, ypos)), module,
                                                    Chrds::INPUT_DRIVER));

        addInput(
            createInputCentered<comp::SIMPort>(mm2px(Vec(right, ypos)), module, Chrds::INPUT_RST));
        float y = ypos + JACKYSPACE;
        float dy = 2.4F;
        for (int i = 0; i < 8; i++) {
            auto* lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((3 * HP), y + i * dy)), module, Chrds::LIGHT_STEP + (i));
            addChild(lli);

            lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((4 * HP), y + i * dy)), module, Chrds::LIGHT_STEP + (8 + i));
            addChild(lli);
        }

        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += 3 * JACKNTXT)),
                                                      module, Chrds::OUTPUT_LDNSS));

        addParam(createParamCentered<comp::SIMSmallKnob>(mm2px(Vec(right, ypos += JACKYSPACE)),
                                                         module, Chrds::PARAM_SHAPE));

        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)), module,
                                                      Chrds::OUTPUT_TRIG));

        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)), module,
                                                      Chrds::OUTPUT_EOC));

        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)), module,
                                                      Chrds::OUTPUT_CV));

        if (!module) { return; }
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Chrds*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);  // NOLINT

        // #ifndef NOPHASOR
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(
            createBoolPtrMenuItem("Disconnect Begin and End", "", &module->disconnectEnds));
        // #endif
        menu->addChild(createBoolPtrMenuItem("Negative 'clk' pulse steps in reverse direction", "",
                                             &module->allowReverseTrigger));
        menu->addChild(
            createBoolPtrMenuItem("Polyphonic Step Trigger output", "", &module->polyStepTrigger));
        menu->addChild(createBoolPtrMenuItem("Polyphonic Volume Correction output", "",
                                             &module->polyLdnssOut));
        menu->addChild(createBoolPtrMenuItem("Polyphonic EOC Trigger", "", &module->polyEOC));
    }
};

Model* modelCrdz = createModel<Chrds, CrdzWidget>("Crdz");  // NOLINT