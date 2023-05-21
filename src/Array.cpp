#include "Rex.hpp"
#include "Segment.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/ParamQuantity.hpp"
#include "helpers.hpp"
#include "jansson.h"
#include "plugin.hpp"

// XXX Snap semitones

struct Array : public biexpand::Expandable {
    enum ParamId {
        ENUMS(PARAM_KNOB, 16),
        PARAMS_LEN

    };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_MAIN, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;
    Array(Array&&) = delete;
    Array& operator=(Array&&) = delete;

   private:
    friend struct ArrayWidget;
    RexAdapter rex;

    constants::VoltageRange voltageRange{constants::ZERO_TO_TEN};
    float minVoltage = 0.0F;
    float maxVoltage = 10.0F;

    enum SnapTo { none, wholeVolts };
    SnapTo snapTo = SnapTo::none;

   public:
    Array() : biexpand::Expandable({{modelReX, &this->rex}}, {})
    // Expandable({modelReX}, {}, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            configParam(PARAM_KNOB + i, 0.0F, 10.F, 0.0F, "Knob", "V");
        }
    }

    void process(const ProcessArgs& /*args*/) override
    {
        const int start = rex.getStart();
        const int length = rex.getLength();
        outputs[OUTPUT_MAIN].setChannels(length);
        for (int i = start; i < start + length; i++) {
            outputs[OUTPUT_MAIN].setVoltage(params[PARAM_KNOB + i].getValue(), i);
        }
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
        // XXX Re-enable
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
