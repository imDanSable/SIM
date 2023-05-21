#pragma once
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct InX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { ENUMS(INPUT_SIGNAL, 16), INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    InX();
};

class InxAdapter : public biexpand::BaseAdapter<InX> {
   public:
    float getVoltage(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getVoltage();
    }
    float getNormalVoltage(int normalVoltage, int port) const
    {
        if (!ptr) { return normalVoltage; }
        return ptr->inputs[port].getNormalVoltage(normalVoltage);
    }
    float getPolyVoltage(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getPolyVoltage(port);
    }

    float getNormalPolyVoltage(float normalVoltage, int channel, int port)
    {
        if (!ptr || !ptr->inputs[port].isConnected()) { return normalVoltage; }
        return ptr->inputs[port].getPolyVoltage(channel);
    }
    bool isConnected(int port) const
    {
        return !(!ptr || !ptr->inputs[port].isConnected());
    }
    int getChannels(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getChannels();
    }
    int getNormalChannels(int normalChannels, int port) const
    {
        if (!ptr) { return normalChannels; }
        return ptr->inputs[port].getChannels();
    }
    int getLastConnectedInputIndex() const
    {
        for (int i = constants::NUM_CHANNELS - 1; i >= 0; i--) {
            if (ptr->inputs[i].isConnected()) { return i; }
        }
        return -1;
    }

    int getFirstConnectedInputIndex() const
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            if (ptr->inputs[i].isConnected()) { return i; }
        }
        return -1;
    }
};
struct InXWidget : ModuleWidget {
    explicit InXWidget(InX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/InX.svg")));

        if (module) { module->addDefaultConnectionLights(this, InX::LIGHT_LEFT_CONNECTED, InX::LIGHT_RIGHT_CONNECTED); }

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addInput(createInputCentered<SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    InX::INPUT_SIGNAL + (i * 8) + j));
            }
        }
    }
};