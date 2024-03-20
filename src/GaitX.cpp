

#include "GaitX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "components.hpp"
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct GaitXWdiget : ModuleWidget {
    explicit GaitXWdiget(GaitX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/GaitX.svg")));

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