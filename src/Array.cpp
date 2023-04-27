#include "Expandable.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::LEFT;
using constants::MAX_GATES;
using constants::NUM_CHANNELS;
using constants::RIGHT;

struct Array : Expandable<Array>
{
    enum ParamId
    {
        ENUMS(PARAM_KNOB, 16),
        PARAMS_LEN

    };
    enum InputId
    {
        INPUTS_LEN
    };
    enum OutputId
    {
        OUTPUT_MAIN,
        OUTPUTS_LEN
    };
    enum LightId
    {
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

    // XXX DOUBLE
    ReX *rex = nullptr; // NOLINT

    Array()
        : Expandable( // XXX DOUBLE
              {modelReX}, {},
              [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
              [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })

    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < constants::NUM_CHANNELS; i++)
        {
            configParam(PARAM_KNOB + i, 0.0F, 10.F, 0.0F, "Knob", "V");
        }
    }
    void updateRightExpanders()
    {
    }
    void updateLeftExpanders()
    {
        // XXX DOUBLE
        rex = updateExpander<ReX, LEFT>({modelReX});
    }

    void process(const ProcessArgs &args) override
    {

        // const bool ui_update = uiTimer.process(args.sampleTime) > constants::UI_UPDATE_TIME;
        const int start = rex ? rex->getStart(0) : 0;
        const int length = rex ? rex->getLength(0) : 16;
        outputs[OUTPUT_MAIN].setChannels(length);
        for (int i = start; i < start + length; i++)
        {
            outputs[OUTPUT_MAIN].setVoltage(params[PARAM_KNOB + i].getValue(), i);
        }
    }
};

using namespace dimensions; // NOLINT
struct ArrayWidget : ModuleWidget
{
    explicit ArrayWidget(Array *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Array.svg")));

        // XXX DOUBLE
        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module,
            Array::LIGHT_LEFT_CONNECTED));
        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec(4 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module,
            Array::LIGHT_RIGHT_CONNECTED));

        addChild(createSegment2x8Widget<Array>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data
            {
                // const int editChannel =
                // static_cast<int>(module->getEditChannel());
                // const int channels =
                //     module->inx != nullptr ?
                //     module->inx->inputs[editChannel].getChannels()
                //     : 16;
                // const int maximum = channels > 0 ? channels :
                // 16; const int active =
                // module->prevChannelIndex[editChannel] %
                // maximum; struct Segment2x8Data segmentdata =
                // {module->editChannelProperties.start,
                //                                      module->editChannelProperties.length,
                //                                      module->editChannelProperties.max,
                //                                      active};
                // XXX DOUBLE
                if (module->rex)
                {
                    return Segment2x8Data{module->rex->getStart(),
                                          module->rex->getLength(), 16, -1};
                }
                return Segment2x8Data{0, 16, 16, -1};
            }));

        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, 16)), module, Array::OUTPUT_MAIN));
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                addParam(createParamCentered<SIMSingleKnob>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    Array::PARAM_KNOB + (i * 8) + j));
            }
        }
    }
};

Model *modelArray = createModel<Array, ArrayWidget>("Array"); // NOLINT

