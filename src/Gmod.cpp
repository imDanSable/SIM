// #include "Debug.hpp"
#include <math.h>

#include <array>
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
    }

    /// @brief Uses clock to calculate the phasor
    /// @return the phase within the clock period and a bool if the clock is triggered
    float processDriver(const ProcessArgs& args, int channel, float cv)
    {
        const bool isClockConnected = inputs[INPUT_DRIVER].isConnected();
        if (!isClockConnected) { return 0.F; }
        /* const bool clockTriggerd = */ clockPhasor[channel].process(args.sampleTime, cv);
        return clockPhasor[channel].getPhase();
    }

    void processDriver(int channel, float cv)
    {
        clockPhasor[channel].setPhase(cv);
    }

    /// @brief Returns the pulse width given the params and cv values
    /// @param channel The channel to get the lengths for
    /// @return The pulse width
    float getPulseWidth(int channel)
    {
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
        const float rawPulseWidth = (denominator == 1.F) ? numerator : numerator / denominator;
        return rawPulseWidth;
    }

    bool processChannel(const ProcessArgs& args,
                        int channel,
                        float driverCv,
                        float triggerCv,
                        int triggerChannels)
    {
        const float phase = usePhasor ? driverCv / 10.F : processDriver(args, channel, driverCv);

        /// DEBUG
        outputs[OUTPUT_GATEOUT].setChannels(2);
        outputs[OUTPUT_GATEOUT].setVoltage(phase, 1);
        /// END DEBUG

        const bool triggered =
            channel >= triggerChannels ? false : triggers[channel].process(triggerCv);

        if (triggered) {
            const bool reverse = !clockPhasor[channel].getDirection();

            const float rawPulseWidth = getPulseWidth(channel);
            // clear the gate window
            gateWindow.clear();
            // add the gate to the window
            gateWindow.add(phase, phase + (reverse ? -rawPulseWidth : rawPulseWidth));
        }
        return gateWindow.get(phase);
        //

        /*
                if (!gateWindow.isEmpty() && gateWindow.allGatesHit() &&
        clockPulseCount[channel] <= 0) { gateWindow.clear();
                    // DEBUG("Clearing gates");
                }

                if (triggered && clockPhasor[channel].isPeriodSet()) {
                    const float rawPulseWidth = getPulseWidth(channel);
                    gateWindow.clear();
                    if (rawPulseWidth > 1.F) {
                        // +1 because a full cycle is two clocks
                        clockPulseCount[channel] = std::floor(rawPulseWidth) + !clockTriggered *
        1;
                    }
                    // debug section
                    if (phase > rack::eucMod(phase + rawPulseWidth, 1.F)) {
                        // DEBUG("Phase: %f, PulseWidth: %f", phase, rawPulseWidth);
                    }
                    else {
                        gateWindow.add(phase, rack::eucMod(phase + rawPulseWidth, 1.F));
                        // DEBUG("Adding gate. FullCycleCounters: %d",
        clockPulseCount[channel]);
                    }
                }
                // Don't count (with get()) if clockPullCount > 0
                const bool gateOn = clockPulseCount[channel] > 0 || gateWindow.get(phase, true);

                if (clockTriggered && clockPulseCount[channel] > 0) {
                    clockPulseCount[channel]--;
                    // DEBUG("Decrementing fullCycleCounters: %d", clockPulseCount[channel]);
                }
        return gateOn;
        */
    }
    void process(const ProcessArgs& args) override
    {
        // Count/set channels
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
    friend struct Gmod_fixture;
    bool usePhasor = false;

    GateWindow gateWindow;
    std::array<ClockPhasor, NUM_CHANNELS> clockPhasor;
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