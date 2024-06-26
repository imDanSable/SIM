#include "InX.hpp"
#include "comp/ports.hpp"
#include "comp/switches.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX() : biexpand::BiExpander(false)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(PARAM_INSERTMODE, 0.0, 2.0, 0.0, "mode",
                 {"Overwrite", "Insert", "Add for voltages (AND for gates)"});
    configCache();
};

struct InXWidget : public SIMWidget {
    explicit InXWidget(InX* module)
    {
        setModule(module);
        setSIMPanel("InX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, -1,
                                                                InX::LIGHT_RIGHT_CONNECTED);
        }

        addParam(createParamCentered<comp::TriModeSwitch>(mm2px(Vec(HP, 15.F)), module,
                                                          InX::PARAM_INSERTMODE));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addInput(createInputCentered<comp::SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    InX::INPUT_SIGNAL + (i * 8) + j));
            }
        }
    }
};
Model* modelInX = createModel<InX, InXWidget>("InX");  // NOLINT