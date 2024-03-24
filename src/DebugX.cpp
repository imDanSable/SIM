#include "DebugX.hpp"
#include "components.hpp"
#include "constants.hpp"  // IWYU pragma: keep
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct DebugXWdiget : ModuleWidget {
    explicit DebugXWdiget(DebugX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/DebugX.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/DebugX.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, DebugX::LIGHT_LEFT_CONNECTED,
                                               DebugX::LIGHT_RIGHT_CONNECTED);
        }

        float ypos{};
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                DebugX::OUTPUT_1));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                DebugX::OUTPUT_2));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                DebugX::OUTPUT_3));
    }
};

Model* modelDebugX = createModel<DebugX, DebugXWdiget>("DebugX");  // NOLINT