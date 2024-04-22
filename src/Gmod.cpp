#include <array>
#include "Debug.hpp"
#include "PhasorAnalyzers.hpp"
#include "Shared.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::NUM_CHANNELS;

struct Gmod : Module {
   public:
    enum ParamId {
        PARAM_LENGTH_DIV,
        PARAM_LENGTH_MUL,
        PARAM_DLY_DIV,
        PARAM_DLY_MUL,
        PARAM_REPS,
        PARAM_REP_FRACTION,
        PARAM_QUANTIZE,
        PARAM_SNAP,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_DRIVER,
        INPUT_TRIGGER,
        INPUT_LENGTH_DIV_CV,
        INPUT_LENGTH_MUL_CV,
        INPUT_DLY_DIV_CV,
        INPUT_DLY_MUL_CV,
        INPUT_REPS_CV,
        INPUT_REP_FRACTIION_CV,
        INPUTS_LEN
    };
    enum OutputId { OUTPUT_GATEOUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    Gmod()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PARAM_LENGTH_DIV, 1.0F, 16.0F, 1.0F, "Length Div");
        configParam(PARAM_LENGTH_MUL, 1.0F, 16.0F, 1.0F, "Length Mul");
        configParam(PARAM_DLY_DIV, 1.0F, 16.0F, 1.0F, "Dly Div");
        configParam(PARAM_DLY_MUL, 1.0F, 16.0F, 1.0F, "Dly Mul");
        configParam(PARAM_REPS, 1.0F, 16.0F, 1.0F, "Reps");
        configParam(PARAM_REP_FRACTION, 0.0F, 1.0F, 0.0F, "Rep Fraction");
        configSwitch(PARAM_QUANTIZE, 0.0F, 1.0F, 1.0F, "Quantize", {"Off", "On"});
        configSwitch(PARAM_SNAP, 0.0F, 1.0F, 0.0F, "Snap", {"Off", "On"});
        for (int i = 0; i < NUM_CHANNELS; i++) {
            gateDetectors[i].setGateWidth(0.F);
            gateDetectors[i].setSmartMode(true);
        }
    }
    /// @brief Uses clock to calculate the phasor
    /// @return the phase within the clock period
    /// 10% dupclicate of Phi
    float timeToPhase(const ProcessArgs& args, int channel, float cv)
    {
        const bool isClockConnected = inputs[INPUT_DRIVER].isConnected();
        if (!isClockConnected) { return 0.F; }

        /* const bool isClockTriggered = */ clockTracker[channel].process(args.sampleTime, cv);
        return clockTracker[channel].getTimeFraction();
    }
    void process(const ProcessArgs& args) override
    {
        /*
        const int channels =
            std::max(inputs[INPUT_DRIVER].getChannels(), inputs[INPUT_TRIGGER].getChannels());
        outputs[OUTPUT_GATEOUT].setChannels(inputs[INPUT_DRIVER].getChannels());
        for (int channel = 0; channel < channels; channel++) {
            const float curCv = inputs[INPUT_DRIVER].getPolyVoltage(channel);
            const float normalizedPhasor =
                !usePhasor ? timeToPhase(args, channel, curCv) : wrappers::wrap(0.1F * curCv);
            const bool triggered =
                channel >= inputs[INPUT_TRIGGER].getChannels()
                    ? false
                    : triggers[channel].process(inputs[INPUT_TRIGGER].getPolyVoltage(channel));
            if (triggered) {
                DEBUG("triggered");
                const float numerator =
                    inputs[INPUT_LENGTH_MUL_CV].isConnected()
                        ? inputs[INPUT_LENGTH_MUL_CV].getPolyVoltage(channel) *
                              rescale(params[PARAM_LENGTH_MUL].getValue(), 1.F, 16.F, 0.F, 1.F)
                        : params[PARAM_LENGTH_MUL].getValue();
                const float denominator =
                    inputs[INPUT_LENGTH_DIV_CV].isConnected()
                        ? inputs[INPUT_LENGTH_DIV_CV].getPolyVoltage(channel) *
                              rescale(params[PARAM_LENGTH_DIV].getValue(), 1.F, 16.F, 0.F, 1.F)
                        : params[PARAM_LENGTH_DIV].getValue();
                const float rawPulseWidth =
                    (denominator == 1.F) ? numerator : numerator / denominator;
                gateDetectors[channel].setGateWidth(rawPulseWidth);
                gateDetectors[channel].setOffset(0.F);
            }
            // pass the new phase to the gate detector (it should return getSmartGate())
            const bool gateOn = gateDetectors[channel](normalizedPhasor);
            outputs[OUTPUT_GATEOUT].setVoltage(gateOn*10.F, channel);
            // outputs[OUTPUT_GATEOUT].setVoltage(normalizedPhasor, channel);
        }
        */
        
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        // json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        // json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        // if (connectEndsJ) { connectEnds = json_is_true(connectEndsJ); }
    }

   private:
    friend struct GmodWidget;
    bool usePhasor = false;
    // std::array<HCVPhasorStepDetector, 16> stepDetectors;
    std::array<HCVPhasorStepDetector, NUM_CHANNELS> stepDetectors;
    std::array<HCVPhasorGateDetector, NUM_CHANNELS> gateDetectors;
    std::array<float, NUM_CHANNELS> gateStartPhases{};
    // std::array<int, NUM_CHANNELS> gateIntPart{};
    std::array<HCVPhasorGateDetector, 16> subGateDetectors;
    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};
    std::array<rack::dsp::SchmittTrigger, NUM_CHANNELS> triggers;
};

using namespace dimensions;  // NOLINT
struct GmodWidget : public SIMWidget {
    explicit GmodWidget(Gmod* module)
    {
        setModule(module);
        setSIMPanel("Gmod");
        float y = 26.F;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Gmod::INPUT_DRIVER));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_TRIGGER));
        y = 43.0F;

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, y)), module, Gmod::PARAM_DLY_DIV));
        addParam(createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_DLY_MUL));
        y += JACKYSPACE;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Gmod::INPUT_DLY_DIV_CV));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_DLY_MUL_CV));

        y = 67.F;

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, y)), module, Gmod::PARAM_LENGTH_DIV));
        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_LENGTH_MUL));
        y += JACKYSPACE;
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Gmod::INPUT_LENGTH_DIV_CV));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_LENGTH_MUL_CV));

        y = 91.F;
        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, y)), module, Gmod::PARAM_REPS));
        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_REP_FRACTION));
        y += JACKYSPACE;
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Gmod::INPUT_REPS_CV));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module,
                                              Gmod::INPUT_REP_FRACTIION_CV));

        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW - 8.F + JACKYSPACE + 7.F)),
                                               module, Gmod::OUTPUT_GATEOUT));

        addParam(createParamCentered<ModeSwitch>(mm2px(Vec(3 * HP, 15.F)), module,
                                                 Gmod::PARAM_QUANTIZE));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Gmod*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);
#ifndef NOPHASOR
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        // menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));
#endif

        menu->addChild(new MenuSeparator);  // NOLINT
    }
};

Model* modelGmod = createModel<Gmod, GmodWidget>("Gmod");  // NOLINT