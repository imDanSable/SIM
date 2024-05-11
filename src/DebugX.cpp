#include "DebugX.hpp"
#include "comp/displays.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"  // IWYU pragma: keep
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct DebugXWidget : public SIMWidget {
    explicit DebugXWidget(DebugX* module)
    {
        setModule(module);
        setSIMPanel("DebugX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, DebugX::LIGHT_LEFT_CONNECTED,
                                                                DebugX::LIGHT_RIGHT_CONNECTED);
        }
        const float right = 5 * HP;

        float ypos{};
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos = JACKYSTART)), module,
                                                      DebugX::OUTPUT_1));
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)), module,
                                                      DebugX::OUTPUT_2));
        addOutput(createOutputCentered<comp::SIMPort>(mm2px(Vec(right, ypos += JACKNTXT)), module,
                                                      DebugX::OUTPUT_3));

        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos = JACKYSTART)), module,
                                                    DebugX::INPUT_START));

        addInput(createInputCentered<comp::SIMPort>(mm2px(Vec(HP, ypos += JACKNTXT)), module,
                                                    DebugX::INPUT_STOP));

        auto* lcd = createWidget<comp::LCDWidget>(mm2px(Vec(HP, ypos += JACKNTXT)));
        lcd->box.size = Vec(50, 11);
        lcd->textGhost = "88";
        if (module)
        lcd->value = &module->frameCount;
        addChild(lcd);
    }
};

void DebugX::process(const ProcessArgs& args)
{
    const bool frameStartTrig = frameStartTrigger.process(inputs[INPUT_START].getVoltage());
    const bool frameStopTrig = frameStopTrigger.process(inputs[INPUT_STOP].getVoltage());

    if (frameStartTrig) { frameCountStart = args.frame; }
    if (frameStopTrig) { frameCount = args.frame - frameCountStart; }
#ifdef DEBUGSTATE
    if (clockDivider.process()) { DebugStateReportAll(); }
#endif
}

Model* modelDebugX = createModel<DebugX, DebugXWidget>("DebugX");  // NOLINT