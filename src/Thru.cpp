#include <array>
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::LEFT;
using constants::RIGHT;

struct Thru : biexpand::Expandable {
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_IN, INPUTS_LEN };
    enum OutputId { OUTPUTS_OUT, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct ThruWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;

   public:
    Thru()
        : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void processOutxNotNormalled(const std::array<float, 16>& inVoltages, int length)
    {
        // Outx not normalled, both snooping and non-snooping
        std::array<bool, 16> snoop = {};
        for (int i = 0; i < length; i++) {
            snoop[i] = outx.setVoltageSnoop(inVoltages[i], i);
        }
        const int snoopedChannelCount = std::count(snoop.begin(), snoop.end(), true);
        const int newLength = length - snoopedChannelCount;
        outputs[OUTPUTS_OUT].setChannels(newLength);
        int currentChannel = 0;
        for (int i = 0; i < length; i++) {
            if (!snoop[i]) {
                outputs[OUTPUTS_OUT].setVoltage(inVoltages[i], currentChannel);
                ++currentChannel;
            }
            outputs[OUTPUTS_OUT].setChannels(currentChannel);
        }
    }

    void processOutxNormalled(const std::array<float, 16>& inVoltages, int length)
    {
        // Outx normalled, both snooping and non-snooping
        std::array<float, 16> normalledVoltages = {};
        int currentChannel = 0;
        for (int i = 0; i < length; i++) {  // Over all input channels
            normalledVoltages[currentChannel] = inVoltages[i];
            currentChannel++;
            if (outx.isConnected(i)) {
                outx.setChannels(currentChannel, i);
                for (int j = 0; j < currentChannel; j++) {  // Over all output channels
                    outx.setVoltage(normalledVoltages[j], i, j);
                }
                currentChannel = 0;
            }
            outputs[OUTPUTS_OUT].setVoltage(inVoltages[i], i);
        }
    }
    void processOutxSnoopedNormalled(const std::array<float, 16>& inVoltages, int length)
    {
        std::array<float, 16> normalledVoltages = {};
        std::array<bool, 16> snoop = {};
        int currentChannel = 0;
        int snoopedChannelIndex = 0;
        for (int i = 0; i < length; i++) {  // Over all input channels
            normalledVoltages[currentChannel] = inVoltages[i];
            currentChannel++;
            if (outx.isConnected(i)) {
                outx.setChannels(currentChannel,
                                 i);  // +1 is taken care of by the currentChannel++ above
                for (int j = 0; j < currentChannel; j++) {  // Over all output channels
                    outx.setVoltage(normalledVoltages[j], i, j);
                    snoop[snoopedChannelIndex++] = true;
                }
                currentChannel = 0;
            }
        }
        const int snoopedChannelCount = std::count(snoop.begin(), snoop.end(), true);
        const int newLength = length - snoopedChannelCount;
        outputs[OUTPUTS_OUT].setChannels(newLength);
        int outChannelIndex = 0;
        for (int i = 0; i < length; i++) {
            if (!snoop[i]) {
                outputs[OUTPUTS_OUT].setVoltage(inVoltages[i], outChannelIndex);
                ++outChannelIndex;
            }
        }
    }
    void process(const ProcessArgs& /*args*/) override
    {
        const int inputChannels = inputs[INPUTS_IN].getChannels();
        if (inputChannels == 0) { return; }
        const int start = rex.getStart();
        const int length = std::min(rex.getLength(), inputChannels);
        outputs[OUTPUTS_OUT].setChannels(length);
        std::array<float, 16> inVoltages = {};
        for (int i = 0, j = start; i < length; i++, j++) {  // Get input voltages
            inVoltages[i] = inputs[INPUTS_IN].getVoltage(j % inputChannels);
        }
        if (!inx && !outx) {
            // No in/out expander, just copy
            outputs[OUTPUTS_OUT].setChannels(length);
            for (int i = 0; i < length; i++) {
                outputs[OUTPUTS_OUT].setVoltage(inVoltages[i], i);
            }
            return;
        }
        if (inx) {
            // Override input voltages with inx expander's voltages
            for (int i = 0; i < length; i++) {
                if (inx.isConnected(i)) { inVoltages[i] = inx.getVoltage(i); }
            }
        }
        if (outx && !outx->getNormalledMode()) {
            processOutxNotNormalled(inVoltages, length);
            return;
        }
        if (outx && !outx->getSnoopMode() && outx->getNormalledMode()) {
            processOutxNormalled(inVoltages, length);
            return;
        }

        if (outx && outx->getSnoopMode() && outx->getNormalledMode()) {
            processOutxSnoopedNormalled(inVoltages, length);
            return;
        }
    }

    enum InxModes { REPLACE, INSERT_BACK, INSERT_FRONT };
};

using namespace dimensions;  // NOLINT

struct ThruWidget : ModuleWidget {
    explicit ThruWidget(Thru* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Thru.svg")));
        if (module) {
            module->addDefaultConnectionLights(this, Thru::LIGHT_LEFT_CONNECTED,
                                               Thru::LIGHT_RIGHT_CONNECTED);
        }

        addInput(createInputCentered<SIMPort>(mm2px(Vec(5.08, 40.0)), module, Thru::INPUTS_IN));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(5.08, 70.0)), module, Thru::OUTPUTS_OUT));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Thru*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
    }
};

Model* modelThru = createModel<Thru, ThruWidget>("Thru");  // NOLINT
