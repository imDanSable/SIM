

#include <array>
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using constants::MAX_STEPS;
using iters::ParamIterator;

struct Bank : biexpand::Expandable {
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
    const int max = MAX_STEPS;

    std::array<bool, MAX_STEPS> bitMemory{};
    bool paramsChanged = true;
    dsp::ClockDivider uiDivider;
    std::vector<bool> v1, v2;
    std::array<std::vector<bool>*, 2> voltages{&v1, &v2};
    std::vector<bool>& readBuffer()
    {
        return *voltages[0];
    }
    std::vector<bool>& writeBuffer()
    {
        return *voltages[1];
    }
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }

    void readVoltages()
    {
        if (paramsChanged) {
            voltages[0]->resize(MAX_STEPS);
            std::copy(bitMemory.begin(), bitMemory.end(), voltages[0]->begin());
            // voltages[0]->assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
            // paramsChanged = false;
        }
    }

    void writeVoltages()
    {
        const int size = voltages[0]->size();
        outputs[OUTPUT_MAIN].setChannels(size);
        for (int channel = 0; channel < size; ++channel) {
            outputs[OUTPUT_MAIN].setVoltage(voltages[0]->at(channel) ? 10.F : 0.F, channel);
        }
    }

   public:
    struct BankParamQuantity : ParamQuantity {
        std::string getString() override
        {
            // Return On or Off instead of 1 or 0
            return ParamQuantity::getValue() > 0.5F ? "On" : "Off";
        };
    };
    Bank()
        : biexpand::Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}},
                               {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configOutput(OUTPUT_MAIN, "Main Output");
        for (int i = 0; i < MAX_STEPS; i++) {
            configParam<BankParamQuantity>(PARAM_BOOL + i, 0.0F, 1.0F, 0.0F,
                                           "Value " + std::to_string(i + 1));
        }
        uiDivider.setDivision(256);

        voltages[0]->resize(MAX_STEPS);
        voltages[1]->resize(MAX_STEPS);
        bitMemory.fill(false);
    }

    void onReset() override
    {
        bitMemory.fill(false);
    }

    template <typename Adapter>
    void perform_transform(Adapter& adapter)
    {
        if (adapter) {
            writeBuffer().resize(MAX_STEPS);
            auto newEnd = adapter.transform(readBuffer().begin(), readBuffer().end(),
                                            writeBuffer().begin(), 0);
            const int channels = std::distance(writeBuffer().begin(), newEnd);
            assert((channels <= 16) && (channels >= 0));  // NOLINT
            writeBuffer().resize(channels);
            swap();
        }
    }

    void performTransforms()
    {
        readVoltages();
        for (biexpand::Adapter* adapter : getLeftAdapters()) {
            perform_transform(*adapter);
        }
        if (outx) { outx.write(readBuffer().begin(), readBuffer().end(), 10.F); }
        perform_transform(outx);
        writeVoltages();
    }
    void onUpdateExpanders(bool /*isRight*/) override
    {
        performTransforms();
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
    void updateUi()
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

        if (length == max) {
            for (int i = 0; i < max; ++i) {
                lights[LIGHTS_BOOL + i].setBrightness(getBitGate(i));
            }
            return;
        }
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

    void process(const ProcessArgs& args) override
    {
        performTransforms();
        if (uiDivider.process()) { updateUi(); }
    }

   private:
    friend struct BankWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;
};

using namespace dimensions;  // NOLINT
struct BankWidget : ModuleWidget {
    explicit BankWidget(Bank* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Bank.svg")));

        // addChild(createSegment2x8Widget<Bank>(
        //     module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
        //     [module]() -> Segment2x8Data {
        //         if (module->rex) {
        //             return Segment2x8Data{, module->rex.getLength(), 16, -1};
        //         }
        //         return Segment2x8Data{0, 16, 16, -1};
        //     }));

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
        addOutput(
            createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW)), module, Bank::OUTPUT_MAIN));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Bank*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT
    }
};

Model* modelBank = createModel<Bank, BankWidget>("Bank");  // NOLINT