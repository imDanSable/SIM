#include "GaitX.hpp"
#include "constants.hpp"  // IWYU pragma: keep
#include "plugin.hpp"
#include "components.hpp"

using namespace dimensions;  // NOLINT
struct GaitXWdiget : ModuleWidget {
    explicit GaitXWdiget(GaitX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/GaitX.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/GaitX.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, GaitX::LIGHT_LEFT_CONNECTED,
                                               GaitX::LIGHT_RIGHT_CONNECTED);
        }

        float ypos{};
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                GaitX::OUTPUT_EOC));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                GaitX::OUTPUT_PHI));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                GaitX::OUTPUT_STEP));
    }
};

Model* modelGaitX = createModel<GaitX, GaitXWdiget>("GaitX");  // NOLINT