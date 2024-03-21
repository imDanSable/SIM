#include "ModX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ModX::ModX()
// ModuleX(false, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configInput(INPUT_PROB, "Probabilities");
    configInput(INPUT_REPS, "Repetitions");
    configInput(INPUT_GLIDE, "Glides");
};

using namespace dimensions;  // NOLINT
struct ModXWdiget : ModuleWidget {
    explicit ModXWdiget(ModX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/ModX.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/ModX.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, ModX::LIGHT_LEFT_CONNECTED,
                                               ModX::LIGHT_RIGHT_CONNECTED);
        }
        float ypos{};
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                              ModX::INPUT_PROB));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                              ModX::INPUT_REPS));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                              ModX::INPUT_GLIDE));
    }
};

Model* modelModX = createModel<ModX, ModXWdiget>("ModX");  // NOLINT