#include "OutX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

struct NormalledModeSwitch : app::SvgSwitch {
    NormalledModeSwitch()
    {
        addFrame(
            Svg::load(asset::plugin(pluginInstance, "res/components/SIMTinyBlueLightSwitch.svg")));
        addFrame(
            Svg::load(asset::plugin(pluginInstance, "res/components/SIMTinyPinkLightSwitch.svg")));
    }
};

struct CutModeSwitch : app::SvgSwitch {
    CutModeSwitch()
    {
        addFrame(
            Svg::load(asset::plugin(pluginInstance, "res/components/SIMTinyBlueLightSwitch.svg")));
        addFrame(
            Svg::load(asset::plugin(pluginInstance, "res/components/SIMTinyPinkLightSwitch.svg")));
    }
};

OutX::OutX()
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(PARAM_NORMALLED, 0.0, 1.0, 0.0, "mode", {"Individual", "Normalled"});
    configSwitch(PARAM_CUT, 0.0, 1.0, 0.0, "mode", {"Copy", "Cut"});
}

// XXX Is this thread safe? and do we really need this?
void OutX::process(const ProcessArgs& /*args*/)
{
    if (leftExpander.module == nullptr) {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            outputs[OUTPUT_SIGNAL + i].setVoltage(0.0F);
            outputs[OUTPUT_SIGNAL + i].setChannels(0);
        }
    }
}

/// @brief Set the voltage of the output and return the cutMode
bool OutxAdapter::setPortVoltage(int outputIndex, float value, int channel)
{
    if (!ptr) { return false; }
    if (ptr->getNormalledMode()) {
        ptr->outputs[lastHigh[channel]].setVoltage(0.F, channel);
        for (int i = outputIndex; i < 16; i++) {
            if (ptr->outputs[i].isConnected()) {
                lastHigh[channel] = i;
                ptr->outputs[i].setVoltage(value, channel);
                return ptr->getCutMode();
            }
        }
    }
    else if (!ptr->getNormalledMode()) {
        if (outputIndex != lastHigh[channel]) {
            ptr->outputs[lastHigh[channel]].setVoltage(0.F, channel);
            lastHigh[channel] = outputIndex;
        }
        if (ptr->outputs[outputIndex].isConnected()) {
            ptr->outputs[outputIndex].setVoltage(value, channel);
            return ptr->getCutMode();
        }
    }
    return false;
}


using namespace dimensions;  // NOLINT
struct OutXWidget : ModuleWidget {
    explicit OutXWidget(OutX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/OutX.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, OutX::LIGHT_LEFT_CONNECTED,
                                               OutX::LIGHT_RIGHT_CONNECTED);
        }

        addParam(createParamCentered<NormalledModeSwitch>(mm2px(Vec(HP, 15.F)), module,
                                                          OutX::PARAM_NORMALLED));
        addParam(createParamCentered<CutModeSwitch>(mm2px(Vec(3 * HP, 15.F)), module,
                                                   OutX::PARAM_CUT));

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