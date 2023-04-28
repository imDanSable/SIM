#include "Expandable.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::LEFT;

// XXX Bug Incorrect display in browser
struct Array : Expandable {
    enum ParamId {
        ENUMS(PARAM_KNOB, 16),
        PARAMS_LEN

    };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_MAIN, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct ArrayWidget;
    friend class Expandable;
    RexAdapter rex;

   public:
    Array() : Expandable({modelReX}, {}, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            configParam(PARAM_KNOB + i, 0.0F, 10.F, 0.0F, "Knob", "V");
        }
    }

    void process(const ProcessArgs& /*args*/) override
    {
        const int start = 0;
        const int length = 16;
        outputs[OUTPUT_MAIN].setChannels(length);
        for (int i = start; i < start + length; i++) {
            outputs[OUTPUT_MAIN].setVoltage(params[PARAM_KNOB + i].getValue(), i);
        }
    }

   private:
    void updateRightExpanders() override {}
    void updateLeftExpanders() override
    {
        // XXX DOUBLE
        rex.setPtr(getExpander<ReX, LEFT>({modelReX}));
    }
};

using namespace dimensions;  // NOLINT
struct ArrayWidget : ModuleWidget {
    explicit ArrayWidget(Array* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Array.svg")));
        if (module) module->addConnectionLights(this);

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
};

Model* modelArray = createModel<Array, ArrayWidget>("Array");  // NOLINT
