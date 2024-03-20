#include "Rex.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ReX::ReX()
// ModuleX(false, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(PARAM_START, 0.0F, 15.0F, 0.0F, "Start", "", 0.0F, 1.0F, 1.0F);
    configParam(PARAM_LENGTH, 1.0F, 16.0F, 16.0F, "Length");

    getParamQuantity(PARAM_START)->snapEnabled = true;
    getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
    configInput(INPUT_START, "Start CV");
    configInput(INPUT_LENGTH, "Length CV");
};

using namespace dimensions;  // NOLINT
struct ReXWidget : ModuleWidget {
    explicit ReXWidget(ReX* module)
    {
        const float center = 1.F * HP;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/Rex.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/Rex.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, ReX::LIGHT_LEFT_CONNECTED,
                                               ReX::LIGHT_RIGHT_CONNECTED);
        }

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
Model* modelReX = createModel<ReX, ReXWidget>("ReX");  // NOLINT
