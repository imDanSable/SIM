#include "OutX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

OutX::OutX()
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

void OutxAdapter::setPortGate(int port, bool gateOn, int channel = 0)
{
    if (!ptr) { return; }
    if (lastPort[channel] != port) {
        ptr->outputs[lastPort[channel]].setVoltage(0.F, channel);
        if (gateOn) { lastPort[channel] = lastPort[channel]; }
        lastPort[channel] = port;
    }
    if (ptr->getNormalledMode()) {
        // Reset previous port on the same channel using lastPort
        // Find the first connected port after port
        int firstConnected = getFirstConnectedIndex(port);
        if (firstConnected == -1) { return; }
        ptr->outputs[firstConnected].setVoltage(10.F * gateOn, channel);
        return;
    }
    ptr->outputs[port].setVoltage(10.F * gateOn, channel);
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
            module->addDefaultConnectionLights(this, OutX::LIGHT_LEFT_CONNECTED,
                                               OutX::LIGHT_RIGHT_CONNECTED);
        }

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