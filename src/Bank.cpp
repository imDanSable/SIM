#include <array>
#include "InX.hpp"
#include "OutX.hpp"
#include "ReX.hpp"
#include "Segment.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "components.hpp"
#include "plugin.hpp"

using constants::MAX_STEPS;
using iters::ParamIterator;

struct Bank : biexpand::Expandable<bool> {
   public:
    enum ParamId { ENUMS(PARAM_BOOL, MAX_STEPS), PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_MAIN, OUTPUTS_LEN };
    enum LightId {

        ENUMS(LIGHTS_BOOL, MAX_STEPS),
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

   private:
    int start = 0;
    int length = MAX_STEPS;
    int max = MAX_STEPS;

    constants::VoltageRange voltageRange{constants::ZERO_TO_TEN};
    float minVoltage = 0.0F;
    float maxVoltage = 10.0F;

    std::array<bool, MAX_STEPS> bitMemory{};
    dsp::ClockDivider uiDivider;

    bool readVoltages(bool forced = false)  // 100% same as Arr
    {
        const bool changed = this->cacheState.needsRefreshing();
        if (changed || forced) {
            readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
            cacheState.refresh();
        }
        return changed;
    }

    void writeVoltages()
    {
        const int size = readBuffer().size();
        outputs[OUTPUT_MAIN].setChannels(size);
        for (int channel = 0; channel < size; ++channel) {
            outputs[OUTPUT_MAIN].setVoltage(readBuffer().at(channel) ? maxVoltage : minVoltage,
                                            channel);
        }
    }

   public:
    struct BankParamQuantity : ParamQuantity {
        std::string getString() override
        {
            return ParamQuantity::getValue() > constants::BOOL_TRESHOLD ? "On" : "Off";
        };
    };
    Bank()
        : biexpand::Expandable<bool>({{modelReX, &this->rex}, {modelInX, &this->inx}},
                                     {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configOutput(OUTPUT_MAIN, "Main Output");
        for (int i = 0; i < MAX_STEPS; i++) {
            configParam<BankParamQuantity>(PARAM_BOOL + i, 0.0F, 1.0F, 0.0F,
                                           "Value " + std::to_string(i + 1));
        }
        configCache();
        uiDivider.setDivision(constants::UI_UPDATE_DIVIDER);

        bitMemory.fill(false);
    }

    void onReset() override
    {
        bitMemory.fill(false);
    }

    void performTransforms(bool forced = false)  // 100% same as Bank
    {
        bool changed = readVoltages(forced);
        bool dirtyAdapters = false;
        if (!changed && !forced) { dirtyAdapters = this->dirtyAdapters(); }
        // Update our buffer because an adapter is dirty and our buffer needs to be updated
        if (!changed && !forced && dirtyAdapters) { readVoltages(true); }

        if (changed || dirtyAdapters || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                transform(*adapter);
            }
            if (outx) { outx.write(readBuffer().begin(), readBuffer().end(), maxVoltage); }
            for (biexpand::Adapter* adapter : getRightAdapters()) {
                transform(*adapter);
            }
            writeVoltages();
        }
    }
    void onUpdateExpanders(bool /*isRight*/) override
    {
        performTransforms(true);
    }

    void setBool(int index, bool value)
    {
        bitMemory[index] = value;
    }
    bool getBool(int index)
    {
        return bitMemory[index];
    }
    void paramToMem()
    {
        for (int i = 0; i < MAX_STEPS; i++) {
            setBool(i, params[PARAM_BOOL + i].getValue() > 0.F);
        }
    }
    void updateUi()  // SPIKE LIKENESS
    {
        start = rex.getStart();
        length = readBuffer().size();
        paramToMem();
        std::array<float, MAX_STEPS> brightnesses{};
        const int start = rex.getStart();
        const int length = readBuffer().size();

        auto getBufGate = [this](int gateIndex) { return (readBuffer()[gateIndex % MAX_STEPS]); };
        auto getBitGate = [this](int gateIndex) { return getBool(gateIndex % MAX_STEPS); };
        const int max = MAX_STEPS;

        for (int i = 0; i < max; ++i) {
            getBitGate(i);
            if (getBitGate(i)) { brightnesses[i] = 0.2F; }
        }
        for (int i = 0; i < length; ++i) {
            if (getBufGate(i)) { brightnesses[(i + start) % max] = 1.F; }
        }
        for (int i = 0; i < max; ++i) {
            lights[LIGHTS_BOOL + i].setBrightness(brightnesses[i]);
        }
    }

    void process(const ProcessArgs& /*args*/) override
    {
        performTransforms();
        if (uiDivider.process()) { updateUi(); }
    }

    int getVoltageRange() const
    {
        return voltageRange;
    }
    void setVoltageRange(constants::VoltageRange range)
    {
        voltageRange = range;
        switch (voltageRange) {
            case constants::ZERO_TO_TEN:
                minVoltage = 0.0F;
                maxVoltage = 10.0F;
                break;
            case constants::ZERO_TO_FIVE:
                minVoltage = 0.0F;
                maxVoltage = 5.0F;
                break;
            case constants::ZERO_TO_THREE:
                minVoltage = 0.0F;
                maxVoltage = 3.0F;
                break;
            case constants::ZERO_TO_ONE:
                minVoltage = 0.0F;
                maxVoltage = 1.0F;
                break;
            case constants::MINUS_TEN_TO_TEN:
                minVoltage = -10.0F;
                maxVoltage = 10.0F;
                break;
            case constants::MINUS_FIVE_TO_FIVE:
                minVoltage = -5.0F;
                maxVoltage = 5.0F;
                break;
            case constants::MINUS_THREE_TO_THREE:
                minVoltage = -3.0F;
                maxVoltage = 3.0F;
                break;
            case constants::MINUS_ONE_TO_ONE:
                minVoltage = -1.0F;
                maxVoltage = 1.0F;
                break;
        }
        performTransforms(true);
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "voltageRange", json_integer(static_cast<int>(voltageRange)));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* voltageRangeJ = json_object_get(rootJ, "voltageRange");
        if (voltageRangeJ) {
            setVoltageRange(
                static_cast<constants::VoltageRange>(json_integer_value(voltageRangeJ)));
        }
    }

   private:
    friend struct BankWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;
};

using namespace dimensions;  // NOLINT
struct BankWidget : public SIMWidget {
    explicit BankWidget(Bank* module)
    {
        setModule(module);
        setSIMPanel("Bank");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, Bank::LIGHT_LEFT_CONNECTED,
                                                                Bank::LIGHT_RIGHT_CONNECTED);
        }
        addChild(createSegment2x8Widget<Bank>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                struct Segment2x8Data segmentdata = {module->start, module->length, module->max,
                                                     -1};  // -1 is out of sight
                return segmentdata;
            }));
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createLightParamCentered<SIMLightLatch<MediumSimpleLight<WhiteLight>>>(
                    mm2px(Vec(HP + (i * 2 * HP),
                              JACKYSTART + (j)*JACKYSPACE)),  // NOLINT
                    module, Bank::PARAM_BOOL + (j + i * 8), Bank::LIGHTS_BOOL + (j + i * 8)));
            }
        }
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW + JACKYSPACE - 9.F)),
                                                module, Bank::OUTPUT_MAIN));
    }

    void appendContextMenu(Menu* menu) override
    {
        SIMWidget::appendContextMenu(menu);

        auto* module = dynamic_cast<Bank*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT

        std::vector<std::string> voltageRangeLabels = {"0V-10V", "0V-5V", "0V-3V", "0V-1V"};
        menu->addChild(createIndexSubmenuItem(
            "Voltage Range", voltageRangeLabels, [module]() { return module->getVoltageRange(); },
            [module](int index) {
                module->setVoltageRange(static_cast<constants::VoltageRange>(index));
            }));
    }
};

Model* modelBank = createModel<Bank, BankWidget>("Bank");  // NOLINT