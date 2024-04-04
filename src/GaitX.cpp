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
                                                                -1);
        }

        float ypos{};
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                GaitX::OUTPUT_EOC));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                GaitX::OUTPUT_PHI));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                GaitX::OUTPUT_STEP));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<GaitX*>(this->module);
        assert(module);  // NOLINT

        // Add a menu for the step output voltage modes
        menu->addChild(createIndexPtrSubmenuItem(
            "Step output voltage",
            {"1/16th per step", "Rescaled 0-10V to length", "1/Length per step"},
            &module->stepOutputVoltageMode));
        SIMWidget::appendContextMenu(menu);
    }
};

Model* modelGaitX = createModel<GaitX, GaitXWdiget>("GaitX");  // NOLINT