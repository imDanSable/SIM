#include "ModX.hpp"
#include "comp/knobs.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"
#include "plugin.hpp"
#include "sp/glide.hpp"

ModX::ModX() : biexpand::BiExpander(false)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configInput(INPUT_PROB, "Randomness");

    configInput(INPUT_REPS, "Repetitions");
    configParam(PARAM_REP_DUR, 0.F, 1.F, .5F, "Repetition duration", "%", 0.F, 100.F);

    configInput(INPUT_GLIDE, "Glides");
    configParam(PARAM_GLIDE_TIME, 0.F, 1.F, .5F, "Glide time", " %", 0.F, 100.F);
    configParam<sp::ShapeQuantity>(PARAM_GLIDE_SHAPE, -1.F, 1.F, 0.F, "Glide shape", "");
    configCache();
};

using namespace dimensions;  // NOLINT
struct ModXWdiget : public SIMWidget {
    explicit ModXWdiget(ModX* module)
    {
        setModule(module);
        setSIMPanel("ModX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, -1,
                                                                ModX::LIGHT_RIGHT_CONNECTED);
        }
        float ypos{};
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                    ModX::INPUT_PROB));
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT + 6.F)), module,
                                                    ModX::INPUT_REPS));
        addParam(createParamCentered<comp::SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                         ModX::PARAM_REP_DUR));
        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT + 6.F)), module,
                                                    ModX::INPUT_GLIDE));
        addParam(createParamCentered<comp::SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                         ModX::PARAM_GLIDE_TIME));
        addParam(createParamCentered<comp::SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                         ModX::PARAM_GLIDE_SHAPE));
    }
};

Model* modelModX = createModel<ModX, ModXWdiget>("ModX");  // NOLINT