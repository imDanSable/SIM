
#include <array>
#include "GaitX.hpp"
#include "InX.hpp"
#include "PhasorAnalyzers.hpp"
#include "ReX.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using namespace constants;  // NOLINT

class Crd : public biexpand::Expandable<float> {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_DRIVER, INPUTS_LEN };
    enum OutputId { OUTPUT_TRIG, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 16), LIGHTS_LEN };

   private:
    friend struct CrdWidget;

    RexAdapter rex;
    InxAdapter inx;
    GaitXAdapter gaitx;

    int numSteps = 0;
    int curStep = 0;
    bool usePhasor = false;
    bool polyStepTrigger = false;
    float gateLength = 1e-3F;

    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger nextTrigger;

    HCVPhasorStepDetector stepDetector;
    HCVPhasorGateDetector gateDetectors;

    dsp::PulseGenerator eocTrigger = {};
    dsp::PulseGenerator newStepTrigger = {};

    bool connectEnds = false;

    std::array<dsp::TTimer<float>, NUM_CHANNELS> nextTimer = {};

    dsp::ClockDivider uiDivider;

    std::span<Light> indicatorLights;

   public:
    Crd()
        : biexpand::Expandable<float>({{modelReX, &rex}, {modelInX, &inx}}, {{modelGaitX, &gaitx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(INPUT_DRIVER, "trigger or phasor input to advance or address step");
        configOutput(OUTPUT_TRIG, "Step trigger");
        configOutput(OUTPUT_CV, "Main");
        configCache({INPUT_DRIVER});
        for (int i = 0; i < NUM_CHANNELS; i++) {
            lights[LIGHT_STEP + i].setBrightness(0.02F);
        }
        uiDivider.setDivision(constants::UI_UPDATE_DIVIDER);
        indicatorLights = {lights.data() + LIGHT_STEP, 16};  // NOLINT
    }

    void onReset() override
    {
        usePhasor = false;
        connectEnds = false;
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
        const int start = rex.getStart();
        for (int lightIdx = 0; lightIdx < MAX_STEPS; ++lightIdx) {
            bool lightOn = false;
            if (((curStep) % numSteps + start) % MAX_STEPS == lightIdx) { lightOn = true; }
            lights[LIGHT_STEP + lightIdx].setBrightness(lightOn ? 1.F : 0.02F);
        }
    }

    void processAuxOutputs(const ProcessArgs& args,
                           float curPhase,
                           bool eoc,
                           int numChannels,
                           bool newStep)
    {
        if (gaitx.getStepConnected()) { gaitx.setStep(curStep, numSteps); }
        const float notePhase = fmodf(curPhase * numSteps, 1.F);
        gaitx.setPhi(notePhase * 10.F);
        if (gaitx.getEOCConnected()) {
            gaitx.setEOC(eocTrigger.process(args.sampleTime) * 10.F);
            if (eoc) { eocTrigger.trigger(1e-3F); }
        }
        const bool trigOutConnected = outputs[OUTPUT_TRIG].isConnected();
        if (trigOutConnected) {
            if (!usePhasor) {
                // Trigger output
                if (newStep) {
                    curStep = (curStep + 1) % numSteps;
                    if (outputs[OUTPUT_TRIG].isConnected()) { newStepTrigger.trigger(gateLength); }
                }
                const bool high = newStepTrigger.process(args.sampleTime);
                outputs[OUTPUT_TRIG].setChannels(polyStepTrigger ? numChannels : 1);
                for (int i = 0; i < numChannels; i++) {
                    outputs[OUTPUT_TRIG].setVoltage(high * 10.F, i);
                }
            }
            else {
                bool high = false;
                gateDetectors.setGateWidth(gateLength);
                gateDetectors.setSmartMode(true);
                const bool gateTrigger = gateDetectors(notePhase);
                outputs[OUTPUT_TRIG].setChannels(polyStepTrigger ? numChannels : 1);
                for (int i = 0; i < numChannels; i++) {
                    outputs[OUTPUT_TRIG].setVoltage((high || gateTrigger) * 10.F, i);
                }
            }
        }
    }

    void processInxIn(const ProcessArgs& args)
    {
        if (!inx) { return; }
        float driverCv = inputs[INPUT_DRIVER].getNormalVoltage(0.F);

        float phase = 0.F;
        bool newStep = false;
        bool eoc = false;
        numSteps = inx.getLastConnectedInputIndex() + 1;
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();

        if (numSteps == 0) { return; }

        if (!usePhasor) {
            newStep = nextTrigger.process(inputs[INPUT_DRIVER].getVoltage());
            eoc = curStep == numSteps - 1;
        }
        else {
            stepDetector.setNumberSteps(numSteps);
            stepDetector.setMaxSteps(PORT_MAX_CHANNELS);
            if (connectEnds) { driverCv = clamp(driverCv, .0f, 9.9999f); }
            phase = clamp(limit(driverCv / 10.F, 1.F), 0.F, 1.F);
            newStep = stepDetector(phase);
            eoc = stepDetector.getEndOfCycle();
            curStep = (stepDetector.getCurrentStep());
        }
        assert(curStep >= 0 && curStep < numSteps);  // NOLINT
        const int numChannels = inx.getChannels(curStep);
        if (cvOutConnected && newStep) {
            outputs[OUTPUT_CV].setChannels(numChannels);
            outputs[OUTPUT_CV].writeVoltages(inx.getVoltages(curStep));
        }
        processAuxOutputs(args, phase, eoc, numChannels, newStep);
    }

    void process(const ProcessArgs& args) override
    {
        if (uiDivider.process()) { updateProgressLights(); }
        const bool driverConnected = inputs[INPUT_DRIVER].isConnected();
        const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
        if (!driverConnected && !cvOutConnected) { return; }
        processInxIn(args);
        gaitx.setChannels(1);
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
        json_object_set_new(rootJ, "polyStepTrigger", json_boolean(polyStepTrigger));
        json_object_set_new(rootJ, "gateLength", json_real(gateLength));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ) { connectEnds = json_is_true(connectEndsJ); }
        json_t* polyStepTriggerJ = json_object_get(rootJ, "polyStepTrigger");
        if (polyStepTriggerJ) { polyStepTrigger = json_is_true(polyStepTriggerJ); }
        json_t* gateLengthJ = json_object_get(rootJ, "gateLength");
        if (gateLengthJ) { gateLength = json_real_value(gateLengthJ); }
    }

   private:
    void onUpdateExpanders(bool isRight) override
    {
        if (!inx) { numSteps = 0; }
    }
};

using namespace dimensions;  // NOLINT

struct CrdWidget : public SIMWidget {
   public:
    struct GateLengthSlider : ui::Slider {
        GateLengthSlider(float* gateLenghtSrc, float minDb, float maxDb)
        {
            quantity = new SliderQuantity<float>(gateLenghtSrc, minDb, maxDb, 1e-3F, "Gate Length",
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

        float ypos = JACKYSTART - JACKNTXT;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, ypos += JACKNTXT)), module,
                                              Crd::INPUT_DRIVER));
        ypos = 60.5F;
        float dy = 2.4F;
        for (int i = 0; i < 8; i++) {
            auto* lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((HP), ypos + i * dy)), module, Crd::LIGHT_STEP + (i));
            addChild(lli);

            lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((2 * HP), ypos + i * dy)), module, Crd::LIGHT_STEP + (8 + i));
            addChild(lli);
        }

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, 87.5F)), module,
                                                Crd::OUTPUT_TRIG));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, 99.F)), module,
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
            createBoolPtrMenuItem("Polyphonic Step Trigger output", "", &module->polyStepTrigger));

        auto* gateLengthSlider = new GateLengthSlider(&(module->gateLength), 1e-3F, 1.F);
        gateLengthSlider->box.size.x = 200.0f;
        menu->addChild(gateLengthSlider);
    }
};

Model* modelCrd = createModel<Crd, CrdWidget>("Crd");  // NOLINT