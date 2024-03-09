#include <array>
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::NUM_CHANNELS;
class Phi : public biexpand::Expandable {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_CV, INPUT_DRIVER, INPUT_RST, INPUTS_LEN };
    enum OutputId { NOTE_PHASE_OUTPUT, OUTPUT_NOTE_INDEX, TRIG_OUTPUT, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, ENUMS(LIGHT_STEP, 32), LIGHTS_LEN };

    enum StepsSource { POLY_IN, INX };

   private:
    friend struct PhiWidget;

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;

    bool usePhasor = false;
    float gateLength = 1e-3F;

    dsp::SchmittTrigger resetTrigger;
    std::array<bool, NUM_CHANNELS> waitingForTriggerAfterReset = {};

    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    std::array<dsp::PulseGenerator, NUM_CHANNELS> trigOutPulses = {};

    //@brief: Adjusts phase per channel so 10V doesn't revert back to zero.
    bool connectEnds = false;
    StepsSource stepsSource = POLY_IN;
    StepsSource preferedStepsSource = POLY_IN;

    //@brief: The channel index last time we process()ed it.
    std::array<int, 16> prevStepIndex = {0};

    //@brief: Pulse generators for gate out

    enum StepOutputVoltageMode {
        SCALE_10_TO_16,
        SCALE_1_TO_10,  // DOCB_COMPATIBLE
        SCALE_10_TO_LENGTH
    };

    StepOutputVoltageMode stepOutputVoltageMode = StepOutputVoltageMode::SCALE_10_TO_16;

   public:
    Phi() : biexpand::Expandable({{modelReX, &rex}, {modelInX, &inx}}, {{modelOutX, &outx}})
    // : Expandable({modelReX, modelInX}, {modelOutX}, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(INPUT_CV, "Poly");
        configInput(INPUT_DRIVER, "Phase clock");
        configOutput(NOTE_PHASE_OUTPUT, "Phase");
        configOutput(OUTPUT_NOTE_INDEX, "Current step");
        configOutput(TRIG_OUTPUT, "Step trigger");
        configOutput(OUTPUT_CV, "Main");
        for (int i = 0; i < NUM_CHANNELS; i++) {
            lights[LIGHT_STEP + i].setBrightness(0.02F);
        }
    }

    void onReset() override
    {
        usePhasor = false;
        connectEnds = false;
        stepsSource = POLY_IN;
        preferedStepsSource = POLY_IN;
        clockTracker.fill({});
        trigOutPulses.fill({});
        prevStepIndex.fill(0);
        waitingForTriggerAfterReset.fill(true);
    }

    void process(const ProcessArgs& args) override
    {
        if (!usePhasor && inputs[INPUT_RST].isConnected() &&
            resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            for (int i = 0; i < NUM_CHANNELS; ++i) {
                clockTracker[i].init();
                trigOutPulses[i].reset();
            }
            prevStepIndex.fill(true);
            waitingForTriggerAfterReset.fill(true);
        }
        const int driverChannels = stepsSource == POLY_IN ? inputs[INPUT_DRIVER].getChannels() : 1;
        for (int currDriverChannel = 0; currDriverChannel < driverChannels; ++currDriverChannel) {
            processChannel(args, currDriverChannel, driverChannels);
        }
        for (int step = 0; step < NUM_CHANNELS; ++step) {
            bool lightOn = false;
            for (int chan = 0; chan < driverChannels; ++chan) {
                if (prevStepIndex[chan] == step) {
                    lightOn = true;
                    break;
                }
            }
            lights[LIGHT_STEP + step].setBrightness(lightOn ? 1.F : 0.02F);
        }
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
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
    struct StartLenMax {  // XXX Double code
        int start = {0};
        int length = {NUM_CHANNELS};
        int max = {NUM_CHANNELS};
    };
    StartLenMax getStartLenMax(int channel) const
    {
        // XXX Add/Fix inx insertmode
        StartLenMax retVal;
        if (stepsSource == POLY_IN) {
            int insertedChannels = 0;
            retVal.max = const_cast<Phi*>(this)->inputs[INPUT_CV].getChannels();  // NOLINT
            if (!rex) {
                if (inx.getInsertMode()) {
                    insertedChannels = inx.getSummedChannels(0, retVal.max + 1);
                }
                retVal.length = clamp(retVal.max + insertedChannels, 0, NUM_CHANNELS);
                return retVal;
            }  // else if (rex)
            retVal.start = rex.getStart(channel, retVal.max);
            retVal.length = rex.getLength(channel, retVal.max);
            insertedChannels = inx.getSummedChannels(retVal.start, retVal.length + 1);
            retVal.length = clamp(retVal.length + insertedChannels, 0, NUM_CHANNELS);
            return retVal;
        }  // else if (stepsSource == INX)
        retVal.max = inx.getLastConnectedInputIndex() + 1;
        if (!rex) {
            retVal.length = retVal.max;
            return retVal;
        }  // else if (rex)
        retVal.start = rex.getStart(channel, retVal.max);
        retVal.length = rex.getLength(channel, retVal.max);
        return retVal;
    }

    void onUpdateExpanders(bool isRight) override
    {
        if (!isRight) { updateStepsSource(); }
    }

    void updateStepsSource()
    {
        // always possible to set to phi
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

    int getMaxChannels() const
    {
        switch (stepsSource) {
            case POLY_IN: return const_cast<uint8_t&>(inputs[INPUT_CV].channels);  // NOLINT
            case INX: return inx.getLastConnectedInputIndex() + 1;
            default: return NUM_CHANNELS;
        }
    }

    int getStepChannels(int idx) const
    {
        switch (stepsSource) {
            case POLY_IN: return const_cast<uint8_t&>(inputs[INPUT_CV].channels);  // NOLINT
            case INX: return inx.getChannels(idx);
            default: return NUM_CHANNELS;
        }
    }

    void processChannel(const ProcessArgs& args, int driverChannel, int maxDriverChannels)
    {
        const StartLenMax startLenMax = getStartLenMax(driverChannel);
        if (startLenMax.length == 0) {
            outputs[OUTPUT_CV].setVoltage(0.F, driverChannel);
            return;
        }

        float cv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0, driverChannel);
        int stepIndex = 0;
        bool triggered = false;
        if (usePhasor) {
            cv = connectEnds ? clamp(cv / 10.F, 0.F, 1.F)
                             : clamp(cv / 10.F, 0.00001F,
                                     .99999f);  // to avoid jumps at 0 and 1
            stepIndex = (startLenMax.start +
                         (static_cast<int>(cv * (startLenMax.length))) % startLenMax.length) %
                        startLenMax.max;
        }
        else {
            triggered = clockTracker[driverChannel].process(args.sampleTime, cv);

            const int addOne = waitingForTriggerAfterReset[driverChannel] ? 0 : 1;
            if (waitingForTriggerAfterReset[driverChannel]) {
                stepIndex = rex.getStart(driverChannel, NUM_CHANNELS);
            }
            else {
                stepIndex = prevStepIndex[driverChannel];
            }

            if (triggered) {  // IMPROVE rewrite if() in terms of %

                if (stepIndex >= startLenMax.start) {
                    stepIndex =
                        ((((stepIndex + addOne) - (startLenMax.start)) % (startLenMax.length)) +
                         startLenMax.start) %
                        startLenMax.max;
                }
                else {
                    stepIndex = ((((stepIndex + addOne + startLenMax.max) - (startLenMax.start)) %
                                  (startLenMax.length)) +
                                 startLenMax.start) %
                                startLenMax.max;
                }
                if (waitingForTriggerAfterReset[driverChannel]) {
                    waitingForTriggerAfterReset[driverChannel] = false;
                }
            }
        }
        // Are we on a new step?
        if ((prevStepIndex[driverChannel] != stepIndex) || triggered) {
            /// TODO: Make a menu option for gate length. Maybe even relgate per channel or per step
            trigOutPulses[driverChannel].trigger(gateLength);
            prevStepIndex[driverChannel] = stepIndex;
        }
        const int outChannelCount =
            stepsSource == POLY_IN ? maxDriverChannels : inx.getChannels(stepIndex);
        outputs[OUTPUT_CV].setChannels(outChannelCount);  // XXX move to process?
        if (stepsSource == POLY_IN) {
            if (!inx.getInsertMode()) {
                const float poly_cv = inputs[INPUT_CV].getNormalPolyVoltage(0, stepIndex);
                const float inx_cv = inx.getNormalPolyVoltage(poly_cv, driverChannel, stepIndex);
                outputs[OUTPUT_CV].setVoltage(inx_cv, driverChannel);
                outx.setVoltage(inx_cv, stepIndex, driverChannel);
            }
            else {  // inx.getInsertMode() // XXXXXXXX Unfinished
                int portCtr = 0;
                int stepCtr = 0;
                int thisPortInxStepsCount = 0;
                int finalStepIndex = stepIndex;
                do {
                    if (inx.isConnected(portCtr)) {
                        if (stepCtr + thisPortInxStepsCount > finalStepIndex) {
                            int indexInThisPort = finalStepIndex - stepCtr;
                            const float inx_cv =
                                inx.getNormalPolyVoltage(0, driverChannel, indexInThisPort);
                            outputs[OUTPUT_CV].setVoltage(inx_cv, driverChannel);
                            outx.setVoltage(inx_cv, portCtr, driverChannel);
                            break;
                        }
                    }
                    else  // if (stepCtr + thisPortInxStepsCount <= finalStepIndex)
                    {
                        stepCtr += thisPortInxStepsCount;
                        portCtr++;
                    }
                } while (true);
            }
        }
        else {  // stepsSource == INX
            for (int i = 0; i < outChannelCount; ++i) {
                const float inx_cv = inx.getNormalPolyVoltage(0, i, stepIndex);
                outputs[OUTPUT_CV].setVoltage(inx_cv, i);
                outx.setVoltage(inx_cv, stepIndex, i);
            }
        }

        // Process STEP OUTPUT
        if (outputs[OUTPUT_NOTE_INDEX].isConnected()) {
            outputs[OUTPUT_NOTE_INDEX].setChannels(maxDriverChannels);
            switch (stepOutputVoltageMode) {
                case SCALE_10_TO_16:
                    outputs[OUTPUT_NOTE_INDEX].setVoltage(0.625 * stepIndex, driverChannel);
                    break;
                case SCALE_1_TO_10:
                    outputs[OUTPUT_NOTE_INDEX].setVoltage((.1F * stepIndex), driverChannel);
                    break;
                case SCALE_10_TO_LENGTH:
                    outputs[OUTPUT_NOTE_INDEX].setVoltage(
                        std::floor(rack::rescale(stepIndex, 0.F, startLenMax.length, 0.F, 10.F)),
                        driverChannel);
                    break;
            }
        }
        // Process NOTE PHASE OUTPUT
        if (outputs[NOTE_PHASE_OUTPUT].isConnected()) {
            outputs[NOTE_PHASE_OUTPUT].setChannels(maxDriverChannels);
            if (usePhasor) {
                const float phase_per_channel = 1.F / startLenMax.length;
                outputs[NOTE_PHASE_OUTPUT].setVoltage(
                    10.F * fmodf(cv, phase_per_channel) / phase_per_channel, driverChannel);
            }
            else {
                if (clockTracker[driverChannel].getPeriodDetected()) {
                    outputs[NOTE_PHASE_OUTPUT].setVoltage(
                        clockTracker[driverChannel].getTimeFraction() * 10.F, driverChannel);
                }
            }
        }
        // Process TRIGGERS
        outputs[TRIG_OUTPUT].setChannels(outChannelCount);
        triggered = trigOutPulses[driverChannel].process(args.sampleTime);
        if (stepsSource == POLY_IN) {
            outputs[TRIG_OUTPUT].setVoltage(10 * triggered, driverChannel);
        }
        else {
            for (int i = 0; i < outChannelCount; ++i) {
                outputs[TRIG_OUTPUT].setVoltage(10 * triggered, i);
            }
        }
    };
};

using namespace dimensions;  // NOLINT

struct PhiWidget : ModuleWidget {
    // Gain adjust menu item
    struct GateLengthQuantity : Quantity {
       private:
        float* gateLengthSrc = nullptr;
        float minSec = 1e-3F;
        float maxSec = 1.f;

       public:
        GateLengthQuantity(float* gate_lenght_src, float min_ms, float max_ms)
            : gateLengthSrc(gate_lenght_src), minSec(min_ms), maxSec(max_ms)
        {
        }
        void setValue(float value) override
        {
            *gateLengthSrc = math::clamp(value, getMinValue(), getMaxValue());
        }
        float getValue() override
        {
            return *gateLengthSrc;
        }
        float getMinValue() override
        {
            return minSec;
        }
        float getMaxValue() override
        {
            return maxSec;
        }
        float getDefaultValue() override
        {
            return 1e-3F;
        }
        float getDisplayValue() override
        {
            return getValue();
        }
        std::string getDisplayValueString() override
        {
            float gateLen = getDisplayValue();
            return string::f("%.3f", gateLen);
        }
        void setDisplayValue(float displayValue) override
        {
            setValue(displayValue);
        }
        std::string getLabel() override
        {
            return "Gate Length";
        }
        std::string getUnit() override
        {
            return " s";
        }
    };
    struct GateLengthSlider : ui::Slider {
        GateLengthSlider(float* gainAdjustSrc, float minDb, float maxDb)
        {
            quantity = new GateLengthQuantity(gainAdjustSrc, minDb, maxDb);
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
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Phi.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART - JACKNTXT)), module,
                                              Phi::INPUT_CV));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART)), module,
                                              Phi::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (1 * JACKNTXT))),
                                              module, Phi::INPUT_RST));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (4 * JACKNTXT))),
                                                module, Phi::NOTE_PHASE_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (5 * JACKNTXT))),
                                                module, Phi::OUTPUT_NOTE_INDEX));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (6 * JACKNTXT))),
                                                module, Phi::TRIG_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (7 * JACKNTXT))),
                                                module, Phi::OUTPUT_CV));
        float y = 48.F;
        float dy = 2.4F;
        for (int i = 0; i < 8; i++) {
            // rack::createLightCentered<rack::TinySimpleLight<rack::componentlibrary::GreenLight>>(
            //     mm2px(Vec((x_offset), y_offset)), this, leftLightId));

            auto* lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((HP), y + i * dy)), module, Phi::LIGHT_STEP + (i));
            addChild(lli);

            lli = createLightCentered<rack::SmallSimpleLight<WhiteLight>>(
                mm2px(Vec((2 * HP), y + i * dy)), module, Phi::LIGHT_STEP + (8 + i));
            addChild(lli);
            // addChild(createLightCentered<>(
            //     Vec(x - dy / 5.f, ly), module, (PolyClone::CHANNEL_LIGHTS + li) * 2));
            // addChild(createLightCentered<MediumLight<YellowRedLight<>>>(
            //     Vec(x + dy / 5.f, ly), module, (PolyClone::CHANNEL_LIGHTS + li + 8) * 2));
        }

        if (!module) { return; }
        module->addDefaultConnectionLights(this, Phi::LIGHT_LEFT_CONNECTED,
                                           Phi::LIGHT_RIGHT_CONNECTED);
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Phi*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));

        menu->addChild(new MenuSeparator);  // NOLINT

        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));

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

        // TODO: Finish. It still can become 0ms (should be 1)
        auto* trackGainAdjustSlider = new GateLengthSlider(&(module->gateLength), 1e-3F, 1.F);
        trackGainAdjustSlider->box.size.x = 200.0f;
        menu->addChild(trackGainAdjustSlider);
    }
};

Model* modelPhi = createModel<Phi, PhiWidget>("Phi");  // NOLINT
