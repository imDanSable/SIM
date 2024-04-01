#include "ReX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ReX::ReX() : biexpand::BiExpander(false)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(PARAM_START, 0.0F, 15.0F, 0.0F, "Start", "", 0.0F, 1.0F, 1.0F);
    configParam(PARAM_LENGTH, 1.0F, 16.0F, 16.0F, "Length");

    getParamQuantity(PARAM_START)->snapEnabled = true;
    getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
    configInput(INPUT_START, "Start CV");
    configInput(INPUT_LENGTH, "Length CV");
    configCache();
};

using namespace dimensions;  // NOLINT
struct ReXWidget : public SIMWidget {
    explicit ReXWidget(ReX* module)
    {
        const float center = 1.F * HP;
        setModule(module);
        setSIMPanel("ReX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, -1,
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
