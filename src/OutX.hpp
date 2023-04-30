#pragma once
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

struct OutX : ModuleX {
    friend struct OutXWidget;
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { ENUMS(OUTPUT_SIGNAL, 16), OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    OutX();
    void process(const ProcessArgs& args) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;
    bool getSnoopMode() const
    {
        return snoopMode;
    }
    bool getNormalledMode() const
    {
        return normalledMode;
    }

   private:
    // XXX Do I need to atomic normaledMode and snoopMode?
    bool normalledMode = false;
    bool snoopMode = false;
};

class OutxAdapter : public BaseAdapter<OutX> {
   public:
    void setVoltage(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return; }
        ptr->outputs[port].setVoltage(voltage, channel);
    }
    bool setVoltageSnoop(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return false; }
        if (!ptr->outputs[port].isConnected()) { return false; }
        ptr->outputs[port].setVoltage(voltage, channel);
        return ptr->getSnoopMode();
    }
    bool isConnected(int port) const
    {
        return !(!ptr || !ptr->outputs[port].isConnected());
    }
    bool setChannels(int channels, int port) const
    {
        if (!ptr) { return false; }
        ptr->outputs[port].setChannels(channels);
        return true;
    }
    int getLastConnectedInputIndex() const
    {
        for (int i = constants::NUM_CHANNELS - 1; i >= 0; i--) {
            if (ptr->outputs[i].isConnected()) { return i; }
        }
        return -1;
    }

    int getFirstConnectedInputIndex() const
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            if (ptr->outputs[i].isConnected()) { return i; }
        }
        return -1;
    }
    bool setExclusiveOutput(int outputIndex, float value, int channel);

   private:
    /// @brief The last output index that was set to a non-zero value per channel
    std::array<int, constants::NUM_CHANNELS> lastHigh = {};
};
using namespace dimensions;  // NOLINT
struct OutXWidget : ModuleWidget {
    explicit OutXWidget(OutX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/OutX.svg")));

        if (module) { module->addConnectionLights(this); }

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
        menu->addChild(createBoolPtrMenuItem("Snoop Mode", "", &module->snoopMode));
    }
};
