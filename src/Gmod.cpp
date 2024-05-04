// #include "Debug.hpp"
#include <array>
#include <cmath>
#include "Shared.hpp"
#include "comp/displays.hpp"
#include "comp/knobs.hpp"
#include "comp/ports.hpp"
#include "comp/switches.hpp"
#include "constants.hpp"
#include "plugin.hpp"
#include "sp/ClockTimer.hpp"
#include "sp/GateWindow.hpp"

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
        configSwitch(PARAM_SNAP, 0.0F, 1.0F, 1.0F, "Snap", {"Off", "On"});
        for (auto& clockPhasor : clockPhasors) {
            clockPhasor.setSteps(steps);
        }
        paramsDivider.setDivision(128);
        updateParams(true);
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

    float getParamValue(int paramId, int inputId, float min, float max, int channel)
    {
        Param& param = params[paramId];
        Port& input = inputs[inputId];
        if (input.isConnected()) {
            return clamp((input.getPolyVoltage(channel) / 10.F) * param.getValue(), min, max);
        }
        return param.getValue();
    }

    float getPulseWidth(int channel)
    {
        const float num = getParamValue(PARAM_LENGTH_MUL, INPUT_LENGTH_MUL_CV, 1, 16, channel);
        const float den = getParamValue(PARAM_LENGTH_DIV, INPUT_LENGTH_DIV_CV, 1, 16, channel);
        return (den == 1.F) ? num : num / den;
    }

    float getPulseDelay(int channel)
    {
        const float num = getParamValue(PARAM_DLY_MUL, INPUT_DLY_MUL_CV, 0, 15, channel);
        const float den = getParamValue(PARAM_DLY_DIV, INPUT_DLY_DIV_CV, 1, 16, channel);
        return (den == 1.F) ? num : num / den;
    }

    void addPulse(float phase, int channel)
    {
        const bool reverse = !clockPhasors[channel].getDirection();
        const float delayedPhase = phase + getPulseDelay(channel) / steps;
        float pulseWidth = getPulseWidth(channel) / steps;
        if (clearOnTrigger) { gateWindows[channel].clear(); }
        gateWindows[channel].add(frac(delayedPhase),
                                 frac(delayedPhase + (reverse ? -limit(pulseWidth, 0.9999f)
                                                              : limit(pulseWidth, 0.9999f))));
    }

    bool processChannel(const ProcessArgs& args,
                        int channel,
                        float driverCv,
                        float triggerCv,
                        int drivingChannels,
                        int triggerChannels,
                        int cvChannels)
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
        bool triggered = false;

        triggered = triggers[channel].process(triggerCv);

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
        gateWindows[channel].removeHitGates();
        return gateWindows[channel].get(phase, true);
    }

    int getCvMaxPoly()
    {
        return std::max(
            {inputs[INPUT_LENGTH_DIV_CV].getChannels(), inputs[INPUT_LENGTH_MUL_CV].getChannels(),
             inputs[INPUT_DLY_DIV_CV].getChannels(), inputs[INPUT_DLY_MUL_CV].getChannels()});
    }
    void process(const ProcessArgs& args) override
    {
        if (paramsDivider.process()) { updateParams(); }
        const int drivingChannels = inputs[INPUT_DRIVER].getChannels();
        const int triggerChannels = inputs[INPUT_TRIGGER].getChannels();
        const int cvChannels = std::max(std::max({inputs[INPUT_LENGTH_DIV_CV].getChannels(),
                                                  inputs[INPUT_LENGTH_MUL_CV].getChannels(),
                                                  inputs[INPUT_DLY_DIV_CV].getChannels(),
                                                  inputs[INPUT_DLY_MUL_CV].getChannels()}),
                                        1);
        const int polyChannels = std::max({drivingChannels, triggerChannels, cvChannels});
        outputs[OUTPUT_GATEOUT].setChannels(polyChannels);

        for (int channel = 0; channel < polyChannels; channel++) {
            const float driverCv = inputs[INPUT_DRIVER].getPolyVoltage(channel);
            const float triggerCv = inputs[INPUT_TRIGGER].getPolyVoltage(channel);
            const bool gateOn = processChannel(args, channel, driverCv, triggerCv, drivingChannels,
                                               triggerChannels, cvChannels);
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
        lenMul = getParamValue(PARAM_LENGTH_MUL, INPUT_LENGTH_MUL_CV, 1, 16, 0);
        lenDiv = getParamValue(PARAM_LENGTH_DIV, INPUT_LENGTH_DIV_CV, 1, 16, 0);
        dlyMul = getParamValue(PARAM_DLY_MUL, INPUT_DLY_MUL_CV, 0, 15, 0);
        dlyDiv = getParamValue(PARAM_DLY_DIV, INPUT_DLY_DIV_CV, 1, 16, 0);
    }
    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "clearOnTrigger", json_integer(clearOnTrigger));
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* clearOnTriggerJ = json_object_get(rootJ, "clearOnTrigger");
        if (clearOnTriggerJ != nullptr) {
            clearOnTrigger = (json_integer_value(clearOnTriggerJ) != 0);
        };
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
    }

   private:
    friend struct GmodWidget;
    float lenMul{}, lenDiv{}, dlyMul{}, dlyDiv{};  // the displays
    constexpr static const float steps = 32.F;     // 16 for duration + 16 for delay
    bool usePhasor = false;
    bool quantize = true;
    bool snap = true;
    bool clearOnTrigger = false;
    rack::dsp::BooleanTrigger snapChanged;
    std::array<sp::GateWindow, 16> gateWindows;
    std::array<bool, NUM_CHANNELS> armed{};
    std::array<sp::ClockPhasor, NUM_CHANNELS> clockPhasors;
    std::array<rack::dsp::SchmittTrigger, NUM_CHANNELS> triggers;
    rack::dsp::ClockDivider paramsDivider;
};

using namespace dimensions;  // NOLINT
struct GmodWidget : public SIMWidget {
    explicit GmodWidget(Gmod* module)
    {
        setModule(module);
        setSIMPanel("Gmod");

        addParam(createParamCentered<comp::ModeSwitch>(mm2px(Vec(1 * HP, 15.F)), module,
                                                       Gmod::PARAM_SNAP));

        addParam(createParamCentered<comp::ModeSwitch>(mm2px(Vec(3 * HP, 15.F)), module,
                                                       Gmod::PARAM_QUANTIZE));
        float y = 26.F;
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, y)), module, Gmod::INPUT_DRIVER));
        addInput(
            createInputCentered<comp::SIMPort>(mm2px(Vec(3 * HP, y)), module, Gmod::INPUT_TRIGGER));
        y = 50.0F;

        auto* lengthDisplay = new comp::RatioDisplayWidget();
        lengthDisplay->box.pos = mm2px(Vec(HP / 2, y - 14.2));  // <---
        lengthDisplay->box.size = mm2px(Vec(3 * HP, 4.F));
        if (module) {
            // make value point to an int pointer
            lengthDisplay->from = &module->lenMul;
            lengthDisplay->to = &module->lenDiv;
        }
        addChild(lengthDisplay);

        addParam(createParamCentered<comp::SIMKnob>(mm2px(Vec(3 * HP, y)), module,
                                                    Gmod::PARAM_LENGTH_DIV));
        addParam(createParamCentered<comp::SIMKnob>(mm2px(Vec(1 * HP, y)), module,
                                                    Gmod::PARAM_LENGTH_MUL));
        y += JACKYSPACE;
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(3 * HP, y)), module,
                                                    Gmod::INPUT_LENGTH_DIV_CV));
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(1 * HP, y)), module,
                                                    Gmod::INPUT_LENGTH_MUL_CV));

        y = 81.F;

        auto* dlyDisplay = new comp::RatioDisplayWidget();
        dlyDisplay->box.pos = mm2px(Vec(HP / 2, y - 14.2));  // <---
        dlyDisplay->box.size = mm2px(Vec(3 * HP, 4.F));
        if (module) {
            // make value point to an int pointer
            dlyDisplay->from = &module->dlyMul;
            dlyDisplay->to = &module->dlyDiv;
        }
        addChild(dlyDisplay);

        addParam(
            createParamCentered<comp::SIMKnob>(mm2px(Vec(3 * HP, y)), module, Gmod::PARAM_DLY_DIV));
        addParam(
            createParamCentered<comp::SIMKnob>(mm2px(Vec(1 * HP, y)), module, Gmod::PARAM_DLY_MUL));
        y += JACKYSPACE;
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(3 * HP, y)), module,
                                                    Gmod::INPUT_DLY_DIV_CV));
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(1 * HP, y)), module,
                                                    Gmod::INPUT_DLY_MUL_CV));

        addChild(createOutputCentered<comp::SIMPort>(
            mm2px(Vec(3 * HP, LOW_ROW - 8.F + JACKYSPACE + 7.F)), module, Gmod::OUTPUT_GATEOUT));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Gmod*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(
            createBoolPtrMenuItem("Clear buffer on Trigger", "", &module->clearOnTrigger));

#ifndef NOPHASOR
        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));
        // menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "",
        // &module->connectEnds));
#endif
    }
};

Model* modelGmod = createModel<Gmod, GmodWidget>("Gmod");  // NOLINT

#ifdef RUNTESTS
// NOLINTBEGIN
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#undef INFO
#undef WARN
#include "test/Catch2/catch_amalgamated.hpp"

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
        int drivingChannels = 1;
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
                gateOn |= gmod.processChannel(args, 0, driverCv, triggerCv, drivingChannels,
                                              triggerChannels, 0.0F);
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
                gateOn |= gmod.processChannel(args, 0, driverCv, triggerCv, drivingChannels,
                                              triggerChannels, 0.0F);
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