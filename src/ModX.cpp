#include "ModX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "glide.hpp"
#include "plugin.hpp"

using namespace glide;  // NOLINT
;
ModX::ModX()
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configInput(INPUT_PROB, "Probabilities");

    configInput(INPUT_REPS, "Repetitions");
    configParam(PARAM_REP_DUR, 0.F, 1.F, .5F, "Repetition duration", "%", 0.F, 100.F);

    configInput(INPUT_GLIDE, "Glides");
    configParam(PARAM_GLIDE_TIME, 0.F, 1.F, .5F, "Glide time", " %", 0.F, 100.F);
    configParam<ShapeQuantity>(PARAM_GLIDE_SHAPE, -1.F, 1.F, 0.F, "Glide shape", "");
    configCache();
};

using namespace dimensions;  // NOLINT
struct ModXWdiget : public SIMWidget {
    explicit ModXWdiget(ModX* module)
    {
        setModule(module);
        setSIMPanel("ModX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, ModX::LIGHT_LEFT_CONNECTED,
                                                                ModX::LIGHT_RIGHT_CONNECTED);
        }
        float ypos{};
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                              ModX::INPUT_PROB));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                              ModX::INPUT_REPS));
        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                   ModX::PARAM_REP_DUR));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                              ModX::INPUT_GLIDE));
        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                   ModX::PARAM_GLIDE_TIME));
        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                   ModX::PARAM_GLIDE_SHAPE));
    }
};

Model* modelModX = createModel<ModX, ModXWdiget>("ModX");  // NOLINT