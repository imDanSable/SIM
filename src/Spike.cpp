#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include "InX.hpp"
#include "OutX.hpp"
#include "RelGate.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/ParamQuantity.hpp"
#include "helpers.hpp"
#include "jansson.h"
#include "plugin.hpp"
#include "ui/MenuSeparator.hpp"

using constants::MAX_GATES;
using constants::NUM_CHANNELS;

struct Spike : biexpand::Expandable {
    Spike(const Spike&) = delete;
    Spike(Spike&&) = delete;
    Spike& operator=(const Spike&) = delete;
    Spike& operator=(Spike&&) = delete;

    enum ParamId { ENUMS(PARAM_GATE, 16), PARAM_DURATION, PARAM_EDIT_CHANNEL, PARAMS_LEN };
    enum InputId { INPUT_CV, INPUT_DURATION_CV, INPUTS_LEN };
    enum OutputId { OUTPUT_GATE, OUTPUTS_LEN };
    enum LightId {
        ENUMS(LIGHTS_GATE, 16),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

    enum PolyphonySource { PHI, REX_CV_START, REX_CV_LENGTH, INX };

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

    bool connectEnds = false;
    PolyphonySource polyphonySource = PHI;
    PolyphonySource preferedPolyphonySource = PHI;
    int singleMemory = 0;  // 0 16 memory banks, 1 1 memory bank

    StartLenMax editChannelProperties = {};
    int prevEditChannel = 0;
    std::array<int, NUM_CHANNELS> prevChannelIndex = {};

    RelGate relGate;

    std::array<float, NUM_CHANNELS> prevCv = {};
    std::array<std::array<bool, 16>, 16> gateMemory = {};

    dsp::Timer uiTimer;

    /// @return true if direction is forward
    static bool getDirection(float cv, float prevCv)
    {
        const float diff = cv - prevCv;
        return ((diff >= 0) && (diff <= 5.F)) || ((-5.F >= diff) && (diff <= 0));
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
        configInput(INPUT_CV, "Φ-in");
        configInput(INPUT_DURATION_CV, "Duration CV");
        configOutput(OUTPUT_GATE, "Trigger/Gate");
        configParam(PARAM_DURATION, 0.1F, 100.F, 100.F, "Gate duration", "%", 0, 1.F);
        configParam(PARAM_EDIT_CHANNEL, 0.F, 15.F, 0.F, "Edit Channel", "", 0, 1.F, 1.F);
        getParamQuantity(PARAM_EDIT_CHANNEL)->snapEnabled = true;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(PARAM_GATE + i, 0.0F, 1.0F, 0.0F, "Gate " + std::to_string(i + 1));
        }
        updatePolyphonySource();
    }

    void onReset() override
    {
        prevCv.fill(0);
        prevChannelIndex.fill(0);
        for (auto& row : gateMemory) {
            row.fill(false);
        }
        connectEnds = false;
        polyphonySource = PHI;
        singleMemory = 0;  // 0 16 memory banks, 1 1 memory bank
        prevEditChannel = 0;
        relGate.reset();
        editChannelProperties = {};
        prevEditChannel = 0;
        prevChannelIndex = {};
        params[PARAM_EDIT_CHANNEL].setValue(0.F);
        params[PARAM_DURATION].setValue(100.F);
        for (int i = 0; i < MAX_GATES; i++) {
            params[PARAM_GATE + i].setValue(0.F);
        }
    }

    void process(const ProcessArgs& args) override
    {
        const int phaseChannelCount = inputs[INPUT_CV].getChannels();

        const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;

        if (ui_update) {
            uiTimer.reset();
            if (phaseChannelCount == 0)  // no input connected. Update ui
            {
                const int editchannel = getEditChannel();
                updateUi(getStartLenMax(editchannel), editchannel);
            }
        }
        if (((polyphonySource == REX_CV_LENGTH) && !rex.cvLengthConnected()) ||
            ((polyphonySource == REX_CV_START) && !rex.cvStartConnected())) {
            polyphonySource = PHI;
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
        json_object_set_new(rootJ, "singleMemory", json_integer(singleMemory));
        json_object_set_new(rootJ, "connectEnds", json_integer(connectEnds));
        json_object_set_new(rootJ, "polyphonySource", json_integer(polyphonySource));
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
        json_t* singleMemoryJ = json_object_get(rootJ, "singleMemory");
        if (singleMemoryJ != nullptr) {
            singleMemory = static_cast<int>(json_integer_value(singleMemoryJ));
        };
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ != nullptr) { connectEnds = (json_integer_value(connectEndsJ) != 0); };
        json_t* polyphonySourceJ = json_object_get(rootJ, "polyphonySource");
        if (polyphonySourceJ != nullptr) {
            polyphonySource = static_cast<PolyphonySource>(json_integer_value(polyphonySourceJ));
            preferedPolyphonySource = polyphonySource;
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
        switch (polyphonySource) {
            case PHI: return const_cast<uint8_t&>(inputs[INPUT_CV].channels);  // NOLINT
            case INX: return inx.getLastConnectedInputIndex() + 1;
            case REX_CV_START: return rex->inputs[ReX::INPUT_START].getChannels();
            case REX_CV_LENGTH: return rex->inputs[ReX::INPUT_LENGTH].getChannels();
            default: return NUM_CHANNELS;
        }
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

    void onUpdateExpanders(bool isRight) override
    {
        if (!isRight) { updatePolyphonySource(); }
    }

    void updatePolyphonySource()
    {
        // always possible to set to phi
        if (preferedPolyphonySource == PHI) {
            polyphonySource = PHI;
            return;
        }
        // Take care of invalid state
        if (!inx && (polyphonySource == INX)) { polyphonySource = PHI; }
        // Take care of invalid state
        if ((!rex && ((polyphonySource == REX_CV_LENGTH) || polyphonySource == REX_CV_START))) {
            polyphonySource = PHI;
        }

        // check prefered sources
        if (preferedPolyphonySource == INX && inx) {
            polyphonySource = INX;
            return;
        }
        if (preferedPolyphonySource == REX_CV_START && rex) {
            polyphonySource = REX_CV_START;
            return;
        }
        if (preferedPolyphonySource == REX_CV_LENGTH && rex) {
            polyphonySource = REX_CV_LENGTH;
            return;
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
            retVal.length = retVal.max;
            return retVal;
        }
        retVal.start = rex.getStart(channel, retVal.max);
        retVal.length = rex.getLength(channel, retVal.max);
        return retVal;
    }

    void processChannel(const ProcessArgs& /*args*/, int channel, int channelCount, bool ui_update)
    {
        StartLenMax startLenMax = getStartLenMax(channel);

        const float cv = inputs[INPUT_CV].getNormalPolyVoltage(0, channel);
        const float phase = connectEnds ? clamp(cv / 10.F, 0.F, 1.F)
                                        : clamp(cv / 10.F, 0.00001F,
                                                .99999f);  // to avoid jumps at 0 and 1
        const bool direction = getDirection(cv, prevCv[channel]);
        prevCv[channel] = cv;
        const int channel_index =
            (((clamp(static_cast<int>(std::floor(static_cast<float>(startLenMax.length) * (phase))),
                     0, startLenMax.length)) +
              startLenMax.start)) %
            (startLenMax.max);

        if ((channel == getEditChannel()) && ui_update) {
            editChannelProperties = getStartLenMax(channel);
            updateUi(editChannelProperties, channel);
        }

        const bool memoryGate = getGate(channel, channel_index);  // NOLINT

        const bool inxOverWrite = inx.isConnected(channel);
        const bool inxGate =
            inxOverWrite &&
            (inx.getNormalPolyVoltage(0, (channel_index % startLenMax.max), channel) > 0);
        if (prevChannelIndex[channel] != channel_index) {
            if (inxGate || memoryGate) {
                const float adjusted_duration =
                    params[PARAM_DURATION].getValue() * 0.1F *
                    inputs[INPUT_DURATION_CV].getNormalPolyVoltage(10.F, channel_index);
                relGate.triggerGate(channel, adjusted_duration, phase, startLenMax.length,
                                    direction);
            }
            prevChannelIndex[channel] = channel_index;
        }

        const bool processGate = relGate.process(channel, phase);
        const bool gate = processGate && (inxOverWrite ? inxGate : memoryGate);
        bool snooped = false;
        if (outx.setChannels(channelCount, channel_index)) {
            snooped = outx.setExclusiveOutput(channel_index, gate ? 10.F : 0.F, channel) && gate;
        }
        outputs[OUTPUT_GATE].setVoltage(snooped ? 0.F : (gate ? 10.F : 0.F), channel);
    }
};

using namespace dimensions;  // NOLINT
struct SpikeWidget : ModuleWidget {
    explicit SpikeWidget(Spike* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spike.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, 16)), module, Spike::INPUT_CV));
        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Spike::OUTPUT_GATE));

        addChild(createSegment2x8Widget<Spike>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                const int editChannel = static_cast<int>(module->getEditChannel());
                const int channels =
                    module->inx ? module->inx->inputs[editChannel].getChannels() : 16;
                const int maximum = channels > 0 ? channels : 16;
                const int active = module->prevChannelIndex[editChannel] % maximum;
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

        std::vector<std::string> operation_modes = {"One memory bank per input",
                                                    "Single shared memory bank"};
        menu->addChild(createIndexSubmenuItem(
            "Gate Memory", operation_modes, [=]() -> int { return module->singleMemory; },
            [=](int index) { module->singleMemory = index; }));

        menu->addChild(createBoolPtrMenuItem("Connect Begin and End", "", &module->connectEnds));

        menu->addChild(createSubmenuItem(
            "Polyphony from",
            [=]() -> std::string {
                switch (module->polyphonySource) {
                    case Spike::PolyphonySource::PHI: return "Φ-in";
                    case Spike::PolyphonySource::REX_CV_START: return "ReX Start CV in";
                    case Spike::PolyphonySource::REX_CV_LENGTH: return "ReX Length CV in";
                    case Spike::PolyphonySource::INX: return "InX last port";
                    default: return "";
                }
                return "";
            }(),
            [=](Menu* menu) {
                menu->addChild(
                    createMenuItem("Φ-in", module->polyphonySource == Spike::PHI ? "✔" : "", [=]() {
                        module->preferedPolyphonySource = Spike::PolyphonySource::PHI;
                        module->updatePolyphonySource();
                    }));
                MenuItem* rex_start_item = createMenuItem(
                    "ReX Start CV in",
                    module->polyphonySource == Spike::PolyphonySource::REX_CV_START ? "✔" : "",
                    [=]() {
                        module->preferedPolyphonySource = Spike::PolyphonySource::REX_CV_START;
                        module->updatePolyphonySource();
                    });
                MenuItem* rex_length_item = createMenuItem(
                    "ReX Length CV in",
                    module->polyphonySource == Spike::PolyphonySource::REX_CV_LENGTH ? "✔" : "",
                    [=]() {
                        module->preferedPolyphonySource = Spike::PolyphonySource::REX_CV_LENGTH;
                        module->updatePolyphonySource();
                    });
                if (!module->rex) {
                    rex_start_item->disabled = true;
                    rex_length_item->disabled = true;
                }
                if (!module->rex.cvStartConnected()) { rex_start_item->disabled = true; }
                if (!module->rex.cvLengthConnected()) { rex_length_item->disabled = true; }
                menu->addChild(rex_start_item);
                menu->addChild(rex_length_item);

                MenuItem* inx_item = createMenuItem(
                    "InX Max Port",
                    module->polyphonySource == Spike::PolyphonySource::INX ? "✔" : "", [=]() {
                        module->preferedPolyphonySource = Spike::PolyphonySource::INX;
                        module->updatePolyphonySource();
                    });
                if (!module->inx) { inx_item->disabled = true; }
                menu->addChild(inx_item);
            }));
    }
};

Model* modelSpike = createModel<Spike, SpikeWidget>("Spike");  // NOLINT