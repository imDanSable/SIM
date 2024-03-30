#include "GaitX.hpp"
#include "components.hpp"
#include "constants.hpp"  // IWYU pragma: keep
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct GaitXWdiget : public SIMWidget {
    explicit GaitXWdiget(GaitX* module)
    {
        setModule(module);
        setSIMPanel("GaitX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, GaitX::LIGHT_LEFT_CONNECTED,
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