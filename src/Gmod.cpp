// #include "Debug.hpp"
#include <array>
#include <cmath>
#include "ClockTimer.hpp"
#include "GateWindow.hpp"
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
        configParam(PARAM_DLY_MUL, 0.F, 15.0F, 0.0F, "Dly Mul");
        configSwitch(PARAM_QUANTIZE, 0.0F, 1.0F, 1.0F, "Quantize", {"Off", "On"});
        configSwitch(PARAM_SNAP, 0.0F, 1.0F, 0.0F, "Snap", {"Off", "On"});
        for (auto& clockPhasor : clockPhasors) {
            clockPhasor.setPeriodFactor(steps);
        }
        paramsDivider.setDivision(128);
        updateParams(true);
    }

    static float getPulsefraction(int channel,
                                  Port& mulCVInput,
                                  Param& mulParam,
                                  Port& divCVInput,
                                  Param& divParam,
                                  bool mulStartsAtOne)
    {
        const float numerator =
            mulCVInput.isConnected()
                ? mulCVInput.getPolyVoltage(channel) *
                      rescale(mulParam.getValue(), mulStartsAtOne, mulStartsAtOne + 15.F, 0.F, 1.F)
                : mulParam.getValue();
        const float denominator = divCVInput.isConnected()
                                      ? divCVInput.getPolyVoltage(channel) *
                                            rescale(divParam.getValue(), 1.F, 16.F, 0.F, 1.F)
                                      : divParam.getValue();
        return (denominator == 1.F) ? numerator : numerator / denominator;
    }

    /// @brief Uses clock to calculate the phasor
    /// @return the phase within the clock period and a bool if the clock is triggered
    std::pair<float, bool> processTimeDriver(const ProcessArgs& args, int channel, float cv)
    {
        const bool isClockConnected = inputs[INPUT_DRIVER].isConnected();
        if (!isClockConnected) { return std::make_pair(0.F, false); }

        const bool clockTriggered = clockPhasors[channel].process(args.sampleTime, cv);
        return std::make_pair(clockPhasors[channel].getPhase(), clockTriggered);
    }

    void addPulse(float phase, int channel)
    {
        const bool reverse = !clockPhasors[channel].getDirection();
        const float delayedPhase =
            phase + getPulsefraction(channel, inputs[INPUT_DLY_MUL_CV], params[PARAM_DLY_MUL],
                                     inputs[INPUT_DLY_DIV_CV], params[PARAM_DLY_DIV], false) /
                        steps;
        float pulseWidth =
            getPulsefraction(channel, inputs[INPUT_LENGTH_MUL_CV], params[PARAM_LENGTH_MUL],
                             inputs[INPUT_LENGTH_DIV_CV], params[PARAM_LENGTH_DIV], true) /
            steps;
        gateWindow.clear();
        gateWindow.add(frac(delayedPhase),
                       frac(delayedPhase +
                            (reverse ? -limit(pulseWidth, 0.9999f) : limit(pulseWidth, 0.9999f))));
    }

    bool processChannel(const ProcessArgs& args,
                        int channel,
                        float driverCv,
                        float triggerCv,
                        int triggerChannels)
    {
        float phase = NAN;
        bool clockTriggered = false;
        if (!usePhasor) {
            std::tie(phase, clockTriggered) = processTimeDriver(args, channel, driverCv);
        }
        else {
            phase = limit(driverCv / 10.F, 1.F);
            clockPhasors[channel].setPhase((driverCv) / 10.F);
            clockTriggered = false;
        }
        const bool triggered =
            channel >= triggerChannels ? false : triggers[channel].process(triggerCv);
        if (triggered) {
            if (quantize) { armed[channel] = true; }
            else {
                addPulse(phase, channel);
            }
        }
        if (clockTriggered && armed[channel]) {
            addPulse(phase, channel);
            armed[channel] = false;
        }

        if (gateWindow.allGatesHit()) { gateWindow.clear(); }
        return gateWindow.get(phase, true);
    }
    void process(const ProcessArgs& args) override
    {
        if (paramsDivider.process()) { updateParams(); }
        const int drivingChannels = inputs[INPUT_DRIVER].getChannels();
        const int triggerChannels = inputs[INPUT_TRIGGER].getChannels();
        const int polyChannels = std::max(drivingChannels, triggerChannels);
        outputs[OUTPUT_GATEOUT].setChannels(inputs[INPUT_DRIVER].getChannels());

        for (int channel = 0; channel < polyChannels; channel++) {
            const float driverCv = inputs[INPUT_DRIVER].getPolyVoltage(channel);
            const float triggerCv = inputs[INPUT_TRIGGER].getPolyVoltage(channel);
            const bool gateOn = processChannel(args, channel, driverCv, triggerCv, triggerChannels);
            outputs[OUTPUT_GATEOUT].setVoltage(gateOn ? 10.F : 0.F, channel);
        }
    }

    void updateParams(bool force = false)
    {
        auto snapParam = [&](int paramId) {
            getParamQuantity(paramId)->snapEnabled = snap;
            params[paramId].setValue(std::round(params[paramId].getValue()));
        };
        snap = params[PARAM_SNAP].getValue() > 0.5F;
        quantize = params[PARAM_QUANTIZE].getValue() > 0.5F;
        if (snapChanged.processEvent(snap) != rack::dsp::BooleanTrigger::NONE || force) {
            snapParam(PARAM_LENGTH_DIV);
            snapParam(PARAM_LENGTH_MUL);
            snapParam(PARAM_DLY_DIV);
            snapParam(PARAM_DLY_MUL);
        }
    }
    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
    }

   private:
    constexpr static const float steps = 32.F;  // 16 for duration + 16 for delay
    friend struct GmodWidget;
    bool usePhasor = false;
    bool quantize = false;
    bool snap = false;
    rack::dsp::BooleanTrigger snapChanged;
    std::array<bool, NUM_CHANNELS> armed{};

    GateWindow gateWindow;
    std::array<ClockPhasor, NUM_CHANNELS> clockPhasors;
    std::array<rack::dsp::SchmittTrigger, NUM_CHANNELS> triggers;
    rack::dsp::ClockDivider paramsDivider;
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

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_DLY_DIV));
        addParam(createParamCentered<SIMKnob>(mm2px(Vec(1 * HP, y)), module, Gmod::PARAM_DLY_MUL));
        y += JACKYSPACE;
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_DLY_DIV_CV));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(1 * HP, y)), module, Gmod::INPUT_DLY_MUL_CV));

        y = 67.F;

        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_LENGTH_DIV));
        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(1 * HP, y)), module, Gmod::PARAM_LENGTH_MUL));
        y += JACKYSPACE;
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_LENGTH_DIV_CV));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(1 * HP, y)), module, Gmod::INPUT_LENGTH_MUL_CV));


        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW - 8.F + JACKYSPACE + 7.F)),
                                               module, Gmod::OUTPUT_GATEOUT));

        addParam(
            createParamCentered<ModeSwitch>(mm2px(Vec(1 * HP, 15.F)), module, Gmod::PARAM_SNAP));

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
        // menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "",
        // &module->connectEnds));
#endif

        menu->addChild(new MenuSeparator);  // NOLINT
    }
};

Model* modelGmod = createModel<Gmod, GmodWidget>("Gmod");  // NOLINT

#ifdef RUNTESTS
// NOLINTBEGIN
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-w"

#include "test/catch_amalgamated.hpp"

class TimePasser {
   public:
    using Action = std::function<void(int)>;
    void reg(Action action)
    {
        actions.push_back(std::move(action));
    }

    void repeat(int reps)
    {
        for (int i = 0; i < reps; i++) {
            for (auto& action : actions) {
                action(frame);
            }
            frame++;
        }
    }

   private:
    long frame = 0;
    std::vector<Action> actions;
};

TEST_CASE("TimePasser tests", "[timepasser]")
{
    SECTION("Action registration and repetition")
    {
        TimePasser timePasser;

        int action1Count = 0;
        int action2Count = 0;
        int finalSum = 0;

        timePasser.reg([&](int) { action1Count++; });
        timePasser.reg([&](int) { action2Count += action1Count; });
        timePasser.reg([&](int) { finalSum = action1Count + action2Count; });

        int repetitions = 5;
        timePasser.repeat(repetitions);

        REQUIRE(action1Count == repetitions);
        REQUIRE(action2Count == repetitions * (repetitions + 1) / 2);
        REQUIRE(finalSum == action1Count + action2Count);
    }
}

SCENARIO("Gmod::process", "[Gmod][processChannel][skip]")
{
    GIVEN("A fresh Gmod module with Gate length = 1/2")
    {
        Gmod gmod;
        // Fake connections
        gmod.inputs[Gmod::INPUT_DRIVER].channels = 1;
        gmod.inputs[Gmod::INPUT_TRIGGER].channels = 1;
        gmod.outputs[Gmod::OUTPUT_GATEOUT].channels = 1;
        gmod.params[Gmod::PARAM_LENGTH_MUL].value = 1.0F;
        gmod.params[Gmod::PARAM_LENGTH_DIV].value = 2.0F;

        Gmod::ProcessArgs args;
        float driverCv = 0.F;
        float triggerCv = 0.F;
        int triggerChannels = 1;
        bool gateOn = false;
        TimePasser tp;
        // Update args
        tp.reg([&args](int frame) {
            args.frame = frame;
            args.sampleTime = 0.1F;
            args.sampleRate = 10.F;
        });
        // Update driverCv  high every frame 15th frame
        tp.reg([&driverCv](int frame) { driverCv = (frame % 15 == 0) ? 1.0F : 0.0F; });

        WHEN("Passing 100 time with a clock trigger every 15th frame")
        {
            tp.reg([&](int frame) {
                gateOn |= gmod.processChannel(args, 0, driverCv, triggerCv, triggerChannels);
            });
            tp.repeat(100);
            THEN("Gate should stay low")
            {
                REQUIRE_FALSE(gateOn);
            }
        }
        WHEN("passing time with a clock trigger every 15th and a trigger every 30th frame")
        {
            tp.reg([&](int frame) {
                triggerCv = (frame % 30 == 0) ? 1.0F : 0.0F;
                gateOn |= gmod.processChannel(args, 0, driverCv, triggerCv, triggerChannels);
            });
            tp.repeat(100);
            THEN("Gate should go high")
            {
                REQUIRE(gateOn);
            }
        }
    }
}
#endif
#pragma GCC diagnostic pop
// NOLINTEND

// #include "test/Gmod_test.hpp"