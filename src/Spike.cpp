
// BUG: Sometimes gatelength 100% will still jitter. Could also be in relgate
// BUG: I think rex-start with rex doesn't progress the current step
// TODO: Thoroughly test new polyphony maxChannel strategy
#include <cmath>

#include <array>
#include <cassert>
#include <vector>
#include "InX.hpp"
#include "OutX.hpp"
#include "RelGate.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;

// XXX How long are the first gates before a period is esteblished?
struct Spike : public biexpand::Expandable {
    enum ParamId { ENUMS(PARAM_GATE, MAX_GATES), PARAM_DURATION, PARAM_EDIT_CHANNEL, PARAMS_LEN };
    enum InputId { DRIVER_CV, INPUT_RST, INPUT_DURATION_CV, INPUTS_LEN };
    enum OutputId { OUTPUT_GATE, OUTPUTS_LEN };
    enum LightId {
        ENUMS(LIGHTS_GATE, MAX_GATES),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

   private:
    friend struct SpikeWidget;

    struct StartLenMax {
        int start = {0};
        int length = {MAX_GATES};
        int max = {MAX_GATES};
    };

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;

    int polyphonyChannels = 1;
    bool usePhasor = false;
    std::array<ClockTracker, NUM_CHANNELS> clockTracker = {};  // used when usePhasor
    dsp::SchmittTrigger resetTrigger;

    bool connectEnds = false;
    int singleMemory = 0;  // 0 16 memory banks, 1 1 memory bank

    StartLenMax editChannelProperties = {};
    int prevEditChannel = 0;
    std::array<int, MAX_GATES> prevGateIndex = {};

    RelGate relGate;

    std::array<float, NUM_CHANNELS> prevCv = {};
    std::array<std::array<bool, 16>, 16> gateMemory = {};

    dsp::Timer uiTimer;

    /// @return true if direction is forward
    static bool getDirection(float cv, float prevCv)
    {
        const float directDiff = cv - prevCv;
        float wrappedDiff = NAN;
        // Calculate wrapped difference considering wraparound
        if (directDiff > .5F) { wrappedDiff = directDiff - 1.F; }
        else if (directDiff < -.5F) {
            wrappedDiff = directDiff + 1.F;
        }
        else {
            wrappedDiff = directDiff;
        }
        return wrappedDiff >= 0.F;
        // Determine direction based on wrapped difference
        // Return true for positive, false for negative
        // if ((diff >= 0) && (diff <= 5.F)) { return true; }
        // return ((diff >= 0) && (diff <= 5.F)) || ((diff >= -5.F) && (diff <= 0));
    };

    /// @return the phase jump since prevCv
    static float getPhaseSpeed(float cv, float prevCv)
    {
        const float diff = cv - prevCv;
        if (diff > 5.F) { return diff - 10.F; }
        if (diff < -5.F) { return diff + 10.F; }
        return diff;
    };

    bool getGate(int channelIndex, int gateIndex, bool ignoreInx = false) const
    {
        assert(channelIndex < constants::NUM_CHANNELS);  // NOLINT
        assert(gateIndex < constants::MAX_GATES);        // NOLINT
        if (!ignoreInx && inx.isConnected(channelIndex)) {
            return inx->inputs[channelIndex].getPolyVoltage(gateIndex) > 0.F;
        }
        if (singleMemory == 0) { return gateMemory[channelIndex][gateIndex]; }
        return gateMemory[0][gateIndex];
    };

    void setGate(int channelIndex, int gateIndex, bool value)
    {
        assert(channelIndex < constants::NUM_CHANNELS);  // NOLINT
        assert(gateIndex < constants::MAX_GATES);        // NOLINT
        if (singleMemory == 0) { gateMemory[channelIndex][gateIndex] = value; }
        else {
            gateMemory[0][gateIndex] = value;
        }
    };

   public:
    Spike()
        : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(DRIVER_CV, "Φ-in");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.1F, 100.F, 100.F, "Gate duration", "%", 0, 1.F);
        configParam(PARAM_EDIT_CHANNEL, 0.F, 15.F, 0.F, "Edit Channel", "", 0, 1.F, 1.F);
        getParamQuantity(PARAM_EDIT_CHANNEL)->snapEnabled = true;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(PARAM_GATE + i, 0.0F, 1.0F, 0.0F, "Gate " + std::to_string(i + 1));
        }
    }

    void onReset() override
    {
        prevCv.fill(0);
        prevGateIndex.fill(0);
        for (auto& row : gateMemory) {
            row.fill(false);
        }
        connectEnds = false;

        singleMemory = 0;  // 0 16 memory banks, 1 1 memory bank
        prevEditChannel = 0;
        relGate.reset();
        editChannelProperties = {};
        prevGateIndex = {};
        params[PARAM_EDIT_CHANNEL].setValue(0.F);
        params[PARAM_DURATION].setValue(100.F);
        for (int i = 0; i < MAX_GATES; i++) {
            params[PARAM_GATE + i].setValue(0.F);
        }
    }

    void process(const ProcessArgs& args) override
    {
        // Reset?
        if (!usePhasor && inputs[INPUT_RST].isConnected() &&
            resetTrigger.process(inputs[INPUT_RST].getVoltage())) {
            for (int channel = 0; channel < NUM_CHANNELS; ++channel) {
                clockTracker[channel].init();
                prevGateIndex[channel] = rex.getStart(channel, MAX_GATES);
                prevCv[channel] = getGate(channel, prevGateIndex[channel], false);
                // if the current channels first gate is high, we need to trigger it.
                if (getGate(channel, prevGateIndex[channel], false)) {
                    // FIXME: This is a hack to make sure the first gate is triggered
                    relGate.triggerGate(channel, 1.F, 1.F);
                }
            }
        }

        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;

        if (ui_update) {
            uiTimer.reset();
            if (polyphonyChannels == 0)  // no input connected. Update ui
            {
                const int editchannel = getEditChannel();
                updateUi(getStartLenMax(editchannel), editchannel);
            }
        }
        const int maxChannels = getMaxChannels();

        outputs[OUTPUT_GATE].setChannels(maxChannels);
        int curr_channel = 0;
        do {
            processChannel(args, curr_channel, maxChannels, ui_update);
            ++curr_channel;
        } while (curr_channel < maxChannels && maxChannels != 0);
        // for (int curr_channel = 0; curr_channel < maxChannels && maxChannels != 0;
        // ++curr_channel) {
        //     processChannel(args, curr_channel, maxChannels, ui_update);
        // }
    };

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "usePhasor", json_integer(usePhasor));
        json_object_set_new(rootJ, "singleMemory", json_integer(singleMemory));
        json_object_set_new(rootJ, "connectEnds", json_integer(connectEnds));
        json_object_set_new(rootJ, "polyphonyChannels", json_integer(polyphonyChannels));
        json_t* patternListJ = json_array();
        for (int k = 0; k < NUM_CHANNELS; k++) {
            json_t* gatesJ = json_array();
            for (int j = 0; j < constants::MAX_GATES; j++) {
                json_array_append_new(gatesJ, json_boolean(gateMemory[k][j]));
            }
            json_array_append_new(patternListJ, gatesJ);
        }
        json_object_set_new(rootJ, "gates", patternListJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* usePhasorJ = json_object_get(rootJ, "usePhasor");
        if (usePhasorJ != nullptr) { usePhasor = (json_integer_value(usePhasorJ) != 0); };
        json_t* singleMemoryJ = json_object_get(rootJ, "singleMemory");
        if (singleMemoryJ != nullptr) {
            singleMemory = static_cast<int>(json_integer_value(singleMemoryJ));
        };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ != nullptr) { connectEnds = (json_integer_value(connectEndsJ) != 0); };
        json_t* polyphonyChannelsJ = json_object_get(rootJ, "polyphonyChannels");
        if (polyphonyChannelsJ != nullptr) {
            polyphonyChannels = static_cast<int>(json_integer_value(polyphonyChannelsJ));
        };
        json_t* data = json_object_get(rootJ, "gates");
        if (!data) { return; }
        for (int k = 0; k < NUM_CHANNELS; k++) {
            json_t* arr = json_array_get(data, k);
            if (arr) {
                for (int j = 0; j < MAX_GATES; j++) {
                    json_t* on = json_array_get(arr, j);
                    gateMemory[k][j] = json_boolean_value(on);
                }
            }
        }
    };

   private:
    int getEditChannel() const
    {
        return clamp(
            static_cast<int>(const_cast<float&>(params[PARAM_EDIT_CHANNEL].value)),  // NOLINT
            0, getMaxChannels() - 1);
    }

    void setEditChannel(int channel)
    {
        params[PARAM_EDIT_CHANNEL].setValue(clamp(channel, 0, getMaxChannels() - 1));
    }

    int getMaxChannels() const
    {
        return polyphonyChannels;
        // XXX from polyphony change
        // return std::max({static_cast<int>(inputs[INPUT_CV].channels),  // NOLINT
        //                  inx.getLastConnectedInputIndex() + 1,
        //                  rex ? rex->inputs[ReX::INPUT_START].getChannels() : 0,
        //                  rex ? rex->inputs[ReX::INPUT_LENGTH].getChannels() : 0});
    }

    void memToParam(int channel)
    {
        assert(channel < constants::NUM_CHANNELS);  // NOLINT
        for (int i = 0; i < MAX_GATES; i++) {
            params[i].setValue(getGate(channel, i, true) ? 1.F : 0.F);
        }
    }

    void paramToMem(int channel)
    {
        assert(channel < constants::NUM_CHANNELS);  // NOLINT
        for (int i = 0; i < MAX_GATES; i++) {
            setGate(channel, i, params[PARAM_GATE + i].getValue() > 0.F);
        }
    }

    void updateUi(const StartLenMax& startLenMax, int channel)
    {
        std::function<bool(int)> getGateLightOn;
        getGateLightOn = [this, channel](int buttonIdx) { return getGate(channel, buttonIdx); };

        if (prevEditChannel != channel) {
            memToParam(channel);
            prevEditChannel = getEditChannel();
        }
        else {
            paramToMem(channel);
        }

        // Reset all lights
        for (int i = 0; i < MAX_GATES; ++i) {
            lights[LIGHTS_GATE + i].setBrightness(0.0F);
        }

        if (startLenMax.length == startLenMax.max) {
            for (int buttonIdx = 0; buttonIdx < startLenMax.max; ++buttonIdx) {
                if (getGateLightOn(buttonIdx)) { lights[buttonIdx].setBrightness(1.F); }
            }
            return;
        }

        // Out of active range
        int end = (startLenMax.start + startLenMax.length - 1) % startLenMax.max;
        int buttonIdx = (end + 1) % startLenMax.max;  // Start beyond the end of the inner range
        while (buttonIdx != startLenMax.start) {
            if (getGateLightOn(buttonIdx)) { lights[LIGHTS_GATE + buttonIdx].setBrightness(0.2F); }
            buttonIdx = (buttonIdx + 1) % startLenMax.max;
        }
        // Active lights
        buttonIdx = startLenMax.start;
        while (buttonIdx != ((end + 1) % startLenMax.max)) {
            if (getGateLightOn(buttonIdx)) { lights[LIGHTS_GATE + buttonIdx].setBrightness(1.F); }
            buttonIdx = (buttonIdx + 1) % startLenMax.max;
        }
    }

    StartLenMax getStartLenMax(int channel) const
    {
        StartLenMax retVal;
        if (!rex && !inx) {
            return retVal;  // 0, MAX_GATES, MAX_GATES
        }
        const int inx_channels = inx.getNormalChannels(NUM_CHANNELS, channel);
        retVal.max = inx_channels == 0 ? NUM_CHANNELS : inx_channels;

        if (!rex && inx) {
            /// XXX Check/test/decide if next line is correct
            retVal.length = retVal.max;
            return retVal;
        }
        retVal.start = rex.getStart(channel, retVal.max);
        retVal.length = rex.getLength(channel, retVal.max);
        return retVal;
    }

    void processChannel(const ProcessArgs& args, int channel, int channelCount, bool ui_update)
    {
        const StartLenMax startLenMax = getStartLenMax(channel);
        if ((channel == getEditChannel()) && ui_update) {
            editChannelProperties = getStartLenMax(channel);
            updateUi(editChannelProperties, channel);
        }
        // Read input (phase or trigger/gate)
        float cv = inputs[DRIVER_CV].getNormalPolyVoltage(0, channel);

        bool direction = true;
        if (usePhasor) {
            // Adjust phase if needed
            cv = connectEnds ? clamp(cv / 10.F, 0.F, 1.F)
                             : clamp(cv / 10.F, 0.00001F,
                                     .99999f);  // to avoid jumps at 0 and 1
            // Determine direction using current and previous phase
            direction = getDirection(cv, prevCv[channel]);
        }
        // Update the current cv in history array
        prevCv[channel] = cv;
        // Calculate the index of the gate based on phase
        int gateIndex = {};
        bool triggered = false;
        if (usePhasor) {
            // XXX: implement and test connectends
            gateIndex =
                (((clamp(
                      static_cast<int>(std::floor(static_cast<float>(startLenMax.length) * (cv))),
                      0, startLenMax.length)) +
                  startLenMax.start)) %
                (startLenMax.max);
        }
        else {
            triggered = clockTracker[channel].process(
                args.sampleTime, inputs[DRIVER_CV].getNormalPolyVoltage(0, channel));

            gateIndex = prevGateIndex[channel];
            if (triggered) {
                if (gateIndex >= startLenMax.start) {
                    gateIndex =
                        ((((gateIndex) - (startLenMax.start)) % (startLenMax.length)) +
                         startLenMax.start) %
                        startLenMax.max;
                }
                else {
                    gateIndex = ((((gateIndex + startLenMax.max) - (startLenMax.start)) %
                                  (startLenMax.length)) +
                                 startLenMax.start) %
                                startLenMax.max;
                }
            }
        }

        // Determine if the gate is on given the memory options
        const bool memoryGate = getGate(channel, gateIndex);  // NOLINT

        // Check if inx is connected
        const bool inxOverWrite = inx.isConnected(channel);
        // See if inx is high
        const bool inxGate = inxOverWrite && (inx.getNormalPolyVoltage(
                                                  0, (gateIndex % startLenMax.max), channel) > 0);
        // Are we on a new gate?
        if ((prevGateIndex[channel] != gateIndex) || triggered) {
            // Is this new gate high?
            if (inxGate || memoryGate) {
                // Calculate the relative duration
                const float relative_duration =
                    params[PARAM_DURATION].getValue() * 0.1F *
                    inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, gateIndex);
                // Update the relative gate class with the new duration
                if (usePhasor) {
                    relGate.triggerGate(channel, relative_duration, cv, startLenMax.length,
                                        direction);
                }
                else {
                    // we add a little delta 1e-3F to the duriation to avoid jitter at 100%
                    // XXX: Maybe we should ad args.sampleTime instead
                    relGate.triggerGate(channel, relative_duration,
                                        clockTracker[channel].getPeriod() + 1e-3F);
                }
            }
            // Update the gate index
            prevGateIndex[channel] = gateIndex;
        }

        // Update the relative gate class with the new phase or
        bool processGate = false;
        if (usePhasor) { processGate = relGate.processPhase(channel, cv); }
        else {
            processGate = relGate.processTrigger(channel, args.sampleTime);
        }
        // Determine if the gate is on
        const bool gateHigh =
            processGate &&
            (inxOverWrite ? inxGate
                          : memoryGate);  // XXX maybe this can go to: Is this new gate high
        // Determine if the gate is cut
        bool gateCut = false;  // XXX maybe this can go to: Is this new gate high
        if (outx.setChannels(channelCount, gateIndex)) {
            gateCut = outx.setPortVoltage(gateIndex, gateHigh ? 10.F : 0.F, channel) && gateHigh;
        }
        // Set the gate output according the gateHigh and gateCut values
        outputs[OUTPUT_GATE].setVoltage(gateCut ? 0.F : (gateHigh ? 10.F : 0.F), channel);
    }
};

using namespace dimensions;  // NOLINT
struct SpikeWidget : ModuleWidget {
    explicit SpikeWidget(Spike* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::DRIVER_CV));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::INPUT_RST));

        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW - JACKNTXT)), module,
                                               Spike::OUTPUT_GATE));

        addChild(createSegment2x8Widget<Spike>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                const int editChannel = static_cast<int>(module->getEditChannel());
                const int channels =
                    module->inx ? module->inx->inputs[editChannel].getChannels() : 16;
                const int maximum = channels > 0 ? channels : 16;
                const int active = module->prevGateIndex[editChannel] % maximum;
                struct Segment2x8Data segmentdata = {module->editChannelProperties.start,
                                                     module->editChannelProperties.length,
                                                     module->editChannelProperties.max, active};
                return segmentdata;
            }));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(
                    mm2px(Vec(HP + (i * 2 * HP),
                              JACKYSTART + (j)*JACKYSPACE)),  // NOLINT
                    module, Spike::PARAM_GATE + (j + i * 8), Spike::LIGHTS_GATE + (j + i * 8)));
            }
        }

        addParam(
            createParamCentered<SIMKnob>(mm2px(Vec(HP, LOW_ROW)), module, Spike::PARAM_DURATION));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, LOW_ROW + JACKYSPACE)), module,
                                              Spike::INPUT_DURATION_CV));

        addParam(createParamCentered<SIMEncoder>(mm2px(Vec(3 * HP, LOW_ROW)), module,
                                                 Spike::PARAM_EDIT_CHANNEL));
        auto* display = new LCDWidget();

        display->box.pos = Vec(35, 318);
        display->box.size = Vec(20, 21);
        display->offset = 1;
        display->textGhost = "18";
        if (module) {
            display->value = &module->prevEditChannel;
            // display->polarity = &module->polarity;
        }

        addChild(display);

        if (module) {
            module->addDefaultConnectionLights(this, Spike::LIGHT_LEFT_CONNECTED,
                                               Spike::LIGHT_RIGHT_CONNECTED);
        }
    }
    void draw(const DrawArgs& args) override
    {
        ModuleWidget::draw(args);
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Spike*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT

        menu->addChild(createBoolPtrMenuItem("Use Phasor as input", "", &module->usePhasor));

        std::vector<std::string> memory_modes = {"One memory bank per input",
                                                 "Single shared memory bank"};
        menu->addChild(createIndexSubmenuItem(
            "Gate Memory", memory_modes, [=]() -> int { return module->singleMemory; },
            [=](int index) { module->singleMemory = index; }));

        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));

        // Add a menu item to set the polyphony channels from 1 to 16 that sets the
        // polyphonyChannels integer

        std::vector<std::string> polyphony_modes = {"1", "2",  "3",  "4",  "5",  "6",  "7",  "8",
                                                    "9", "10", "11", "12", "13", "14", "15", "16"};

        menu->addChild(createIndexSubmenuItem(  // NOLINT
            "Polyphony Channels", polyphony_modes,
            [=]() -> int { return module->polyphonyChannels - 1; },
            [=](int index) { module->polyphonyChannels = index + 1; }));

        // TODO: Change this into setting it manually
    }
};

Model* modelSpike = createModel<Spike, SpikeWidget>("Spike");  // NOLINT