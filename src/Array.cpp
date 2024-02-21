#include <iterator>
#include <variant>
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/ParamQuantity.hpp"
#include "helpers.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using iters::ParamIterator;

// XXX Snap semitones
// XXX I saw array snap when restarted
// Make Arr works with OutX including normalled mode and snoop mode?
struct Array : public biexpand::Expandable {
    enum ParamId {
        ENUMS(PARAM_KNOB, 16),
        PARAMS_LEN

    };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_MAIN, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    bool paramsChanged = true;  // XXX implement
    std::vector<float> v1, v2;
    std::array<std::vector<float>*, 2> voltages{&v1, &v2};
    std::vector<float>& readBuffer()
    {
        return *voltages[0];
    }
    std::vector<float>& writeBuffer()
    {
        return *voltages[1];
    }
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }
    friend struct ArrayWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;

    constants::VoltageRange voltageRange{constants::ZERO_TO_TEN};
    float minVoltage = 0.0F;
    float maxVoltage = 10.0F;

    enum SnapTo { none, wholeVolts };
    SnapTo snapTo = SnapTo::none;

   public:
    Array()
        : biexpand::Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}},
                               {{modelOutX, &this->outx}})
    {
        v1.resize(16);
        v2.resize(16);
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            configParam(PARAM_KNOB + i, 0.0F, 10.F, 0.0F, "Knob", "V");
        }
    }
    template <typename Adapter>
    void perform_transform(Adapter& adapter)
    {
        if (adapter) {
            auto newEnd =
                adapter.transform(readBuffer().begin(), readBuffer().end(), writeBuffer().begin());
            const int channels = std::distance(writeBuffer().begin(), newEnd);
            assert((channels <= 16) && (channels >= 0));
            writeBuffer().resize(channels);
            swap();
        }
    }

    void performTransforms()
    {
        readVoltages();
        //XXX We'd like something general like this:
        // for (biexpand::Adapter* adapter : getLeftAdapters()) {
        //     perform_transform(*adapter);
        // }
        // So that we can deal with the order of expanders
        perform_transform(rex);
        perform_transform(inx);
        if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
        perform_transform(outx);
        writeVoltages();
    }
    void onUpdateExpanders(bool isRight) override
    {
        performTransforms();
    }
    void process(const ProcessArgs& /*args*/) override
    {
        performTransforms();
    }

    SnapTo getSnapTo() const
    {
        return snapTo;
    }

    void setSnapTo(SnapTo snapTo)
    {
        this->snapTo = snapTo;
        bool snap = snapTo != SnapTo::none;
        for (int i = PARAM_KNOB; i < PARAM_KNOB + constants::NUM_CHANNELS; i++) {
            getParamQuantity(i)->snapEnabled = snap;
            getParam(i).setValue(std::round(getParam(i).getValue()));
        }
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
        for (int i = PARAM_KNOB; i < PARAM_KNOB + constants::NUM_CHANNELS; i++) {
            const float old_value = getParam(i).getValue();
            ParamQuantity* pq = getParamQuantity(i);
            const float old_proportion = (old_value - pq->minValue) / (pq->maxValue - pq->minValue);
            pq->minValue = minVoltage;
            pq->maxValue = maxVoltage;
            getParam(i).setValue(pq->minValue + old_proportion * (pq->maxValue - pq->minValue));
        }
    }
    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "voltageRange", json_integer(static_cast<int>(voltageRange)));
        json_object_set_new(rootJ, "snapTo", json_integer(static_cast<int>(snapTo)));
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            json_object_set_new(rootJ, ("knob" + std::to_string(i)).c_str(),
                                json_real(getParam(PARAM_KNOB + i).getValue()));
        }
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* voltageRangeJ = json_object_get(rootJ, "voltageRange");
        if (voltageRangeJ) {
            setVoltageRange(
                static_cast<constants::VoltageRange>(json_integer_value(voltageRangeJ)));
        }
        json_t* snapToJ = json_object_get(rootJ, "snapTo");
        if (snapToJ) { setSnapTo(static_cast<SnapTo>(json_integer_value(snapToJ))); }
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            json_t* knobJ = json_object_get(rootJ, ("knob" + std::to_string(i)).c_str());
            if (knobJ) {
                getParam(PARAM_KNOB + i).setValue(json_real_value(knobJ));
            }
        }
    }

   private:
    void readVoltages()
    {
        if (paramsChanged) {
            voltages[0]->assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
            // paramsChanged = false; // XXX implement
        }
    }

    void writeVoltages()
    {
        outputs[OUTPUT_MAIN].setChannels(voltages[0]->size());
        outputs[OUTPUT_MAIN].writeVoltages(voltages[0]->data());
    }
};

using namespace dimensions;  // NOLINT
struct ArrayWidget : ModuleWidget {
    explicit ArrayWidget(Array* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Array.svg")));
        if (module) {
            module->addDefaultConnectionLights(this, Array::LIGHT_LEFT_CONNECTED,
                                               Array::LIGHT_RIGHT_CONNECTED);
        }

        addChild(createSegment2x8Widget<Array>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                if (module->rex) {
                    return Segment2x8Data{module->rex.getStart(), module->rex.getLength(), 16, -1};
                }
                return Segment2x8Data{0, 16, 16, -1};
            }));

        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Array::OUTPUT_MAIN));
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createParamCentered<SIMSingleKnob>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    Array::PARAM_KNOB + (i * 8) + j));
            }
        }
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Array*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT
        std::vector<std::string> snapToLabels = {"None", "Octave (1V)"};
        menu->addChild(createIndexSubmenuItem(
            "Snap to", snapToLabels, [module]() { return module->getSnapTo(); },
            [module](int index) { module->setSnapTo(static_cast<Array::SnapTo>(index)); }));

        std::vector<std::string> voltageRangeLabels = {"0V-10V", "0V-5V", "0V-3V", "0V-1V",
                                                       "+/-10V", "+/-5V", "+/-3V", "+/-1V"};
        menu->addChild(createIndexSubmenuItem(
            "Voltage Range", voltageRangeLabels, [module]() { return module->getVoltageRange(); },
            [module](int index) {
                module->setVoltageRange(static_cast<constants::VoltageRange>(index));
            }));
    }
};

Model* modelArray = createModel<Array, ArrayWidget>("Array");  // NOLINT
