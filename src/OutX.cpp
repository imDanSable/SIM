#include "OutX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

OutX::OutX()
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
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

json_t* OutX::dataToJson()
{
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "cutMode", json_boolean(cutMode));
    json_object_set_new(rootJ, "normalledMode", json_boolean(normalledMode));
    return rootJ;
}
void OutX::dataFromJson(json_t* rootJ)
{
    json_t* cutModeJ = json_object_get(rootJ, "cutMode");
    if (cutModeJ) { cutMode = json_boolean_value(cutModeJ); }
    json_t* normalledModeJ = json_object_get(rootJ, "normalledMode");
    if (normalledModeJ) { normalledMode = json_boolean_value(normalledModeJ); }
};

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

        int id = 0;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addOutput(createOutputCentered<SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module, id++));
            }
        }
    }

    void appendContextMenu(Menu* menu) override
    {
        OutX* module = dynamic_cast<OutX*>(this->module);
        assert(module);                     // NOLINT
        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(createBoolPtrMenuItem("Normalled Mode", "", &module->normalledMode));
        menu->addChild(createBoolPtrMenuItem("Cut Mode", "", &module->cutMode));
    }
};
Model* modelOutX = createModel<OutX, OutXWidget>("OutX");  // NOLINT