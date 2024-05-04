#include "DebugX.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"  // IWYU pragma: keep
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct DebugXWdiget : public SIMWidget {
    explicit DebugXWdiget(DebugX* module)
    {
        setModule(module);
        setSIMPanel("DebugX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, DebugX::LIGHT_LEFT_CONNECTED,
                                                                DebugX::LIGHT_RIGHT_CONNECTED);
        }

        float ypos{};
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                      DebugX::OUTPUT_1));
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                      DebugX::OUTPUT_2));
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                      DebugX::OUTPUT_3));
    }
};

Model* modelDebugX = createModel<DebugX, DebugXWdiget>("DebugX");  // NOLINT