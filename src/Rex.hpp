#pragma once
#include <variant>
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/Module.hpp"
#include "plugin.hpp"

struct ReX : public ModuleX {
    enum ParamId { PARAM_START, PARAM_LENGTH, PARAMS_LEN };
    enum InputId { INPUT_START, INPUT_LENGTH, INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    ReX();
};

class RexAdapter : public BaseAdapter<ReX> {
    std::variant<ReX*, Module*> ptr2;
   public:
    bool cvStartConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_START].isConnected();
    }

    bool cvLengthConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_LENGTH].isConnected();
    }

    int getLength(int channel = 0, int max = constants::NUM_CHANNELS) const
    {
        if (!ptr) { return constants::NUM_CHANNELS; }
        if (!ptr->inputs[ReX::INPUT_LENGTH].isConnected()) {
            return clamp(static_cast<int>(ptr->params[ReX::PARAM_LENGTH].getValue()), 1, max);
        }

        return clamp(static_cast<int>(rescale(ptr->inputs[ReX::INPUT_LENGTH].getVoltage(channel), 0,
                                              10, 1, static_cast<float>(max + 1))),
                     1, max);
    };
    int getStart(int channel = 0, int max = constants::NUM_CHANNELS) const
    {
        if (!ptr) { return 0; }
        if (!ptr->inputs[ReX::INPUT_START].isConnected()) {
            return clamp(static_cast<int>(ptr->params[ReX::PARAM_START].getValue()), 0, max - 1);
        }
        return clamp(static_cast<int>(rescale(ptr->inputs[ReX::INPUT_START].getPolyVoltage(channel),
                                              0, 10, 0, static_cast<float>(max))),
                     0, max - 1);
    };
};

using namespace dimensions;  // NOLINT
struct ReXWidget : ModuleWidget {
    explicit ReXWidget(ReX* module)
    {
        const float center = 1.F * HP;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Rex.svg")));

        if (module) module->addConnectionLights(this);

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT)),
                                              module, ReX::PARAM_START));
        addInput(createInputCentered<SIMPort>(
            mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT + JACKYSPACE)), module,
            ReX::INPUT_START));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT)),
                                              module, ReX::PARAM_LENGTH));
        addInput(createInputCentered<SIMPort>(
            mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT + JACKYSPACE)), module,
            ReX::INPUT_LENGTH));
    }
};
