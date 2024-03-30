#include "InX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX()
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(PARAM_INSERTMODE, 0.0, 1.0, 0.0, "mode", {"Overwrite", "Insert"});
    configCache();
};

struct InXWidget : public SIMWidget {
    explicit InXWidget(InX* module)
    {
        setModule(module);
        setSIMPanel("InX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, InX::LIGHT_LEFT_CONNECTED,
                                                                InX::LIGHT_RIGHT_CONNECTED);
        }

        addParam(
            createParamCentered<ModeSwitch>(mm2px(Vec(HP, 15.F)), module, InX::PARAM_INSERTMODE));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addInput(createInputCentered<SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    InX::INPUT_SIGNAL + (i * 8) + j));
            }
        }
    }
};
Model* modelInX = createModel<InX, InXWidget>("InX");  // NOLINT