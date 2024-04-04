// SOMEDAYMAYBE: make a cut cache
#include "OutX.hpp"
#include "Segment.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

OutX::OutX() : biexpand::BiExpander(true)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(PARAM_NORMALLED, 0.0, 1.0, 0.0, "mode", {"Individual", "Normalled"});
    configSwitch(PARAM_CUT, 0.0, 1.0, 0.0, "mode", {"Copy", "Cut"});
    configCache();
}

void OutX::process(const ProcessArgs& /*args*/)
{
    if (leftExpander.module == nullptr) {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            outputs[OUTPUT_SIGNAL + i].setVoltage(0.0F);
            outputs[OUTPUT_SIGNAL + i].setChannels(0);
        }
    }
}

bool OutxAdapter::writeGateVoltage(int port, bool gateOn, int channel)
{
    if (!ptr) { return false; }
    // // Reset previous port on the same channel using lastPort
    if (lastPort[channel] != port) {
        ptr->outputs[lastPort[channel]].setVoltage(0.F, channel);
        if (gateOn) { lastPort[channel] = lastPort[channel]; }
        lastPort[channel] = port;
    }
    if (!ptr->getNormalledMode() && ptr->outputs[port].isConnected()) {
        ptr->outputs[port].setVoltage(gateOn * 10.F, channel);
        return ptr->getCutMode();
    }
    // Normalled mode
    for (int portIdx = port; portIdx < 16; portIdx++) {
        if (ptr->outputs[portIdx].isConnected()) {
            ptr->outputs[portIdx].setVoltage(gateOn * 10.F, channel);
            return ptr->getCutMode();
        }
    }
    return false;
}

/// @brief report how many items will be cut
int OutxAdapter::totalConnected(int /*channel*/) const
{
    if (!ptr) { return 0; }
    if (ptr->getNormalledMode()) { return getLastConnectedIndex(); }
    int count = 0;
    for (int i = 0; i < 16; i++) {
        if (ptr->outputs[i].isConnected()) { count++; }
    }
    return count;
}
using namespace dimensions;  // NOLINT
struct OutXWidget : public SIMWidget {
    explicit OutXWidget(OutX* module)
    {
        setModule(module);
        setSIMPanel("OutX");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, OutX::LIGHT_LEFT_CONNECTED,
                                                                -1);
        }
        addChild(createSegment2x8Widget<OutX>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                return Segment2x8Data{0, module->getLastNormalledPortIndex(), 16, -1};
            }));
        addParam(
            createParamCentered<ModeSwitch>(mm2px(Vec(HP, 15.F)), module, OutX::PARAM_NORMALLED));
        addParam(
            createParamCentered<ModeSwitch>(mm2px(Vec(3 * HP, 15.F)), module, OutX::PARAM_CUT));

        int id = 0;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addOutput(createOutputCentered<SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module, id++));
            }
        }
    }
};
Model* modelOutX = createModel<OutX, OutXWidget>("OutX");  // NOLINT