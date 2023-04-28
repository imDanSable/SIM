#include "Expandable.hpp"
#include "Rex.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::LEFT;
using constants::RIGHT;

struct Thru : Expandable<Thru> {
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_IN, INPUTS_LEN };
    enum OutputId { OUTPUTS_OUT, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    Thru()
        : Expandable(  // XXX DOUBLE
              {modelReX, modelInX},
              {modelOutX},
              [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
              [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void process(const ProcessArgs& /*args*/) override
    {
        const int inputChannels = inputs[INPUTS_IN].getChannels();
        if (inputChannels == 0) { return; }
        const int start = rex.getStart();
        const int length = rex.getLength();
        const int outputChannels = std::min(inputChannels, length);
        outputs[OUTPUTS_OUT].setChannels(outputChannels);
        for (int i = start, j = 0; i < start + length; i++, j++) {
            outputs[OUTPUTS_OUT].setVoltage(inputs[INPUTS_IN].getVoltage(i % inputChannels), j);
        }
    }
    void updateRightExpanders() {}
    void updateLeftExpanders()
    {
        // XXX DOUBLE
        rex.setReX(updateExpander<ReX, LEFT>({modelReX}));
    }

   private:
    friend struct ThruWidget;
    friend class Expandable<Thru>;
    RexAdapter rex;
};

using namespace dimensions;  // NOLINT

struct ThruWidget : ModuleWidget {
    explicit ThruWidget(Thru* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Thru.svg")));

        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module,
            Thru::LIGHT_LEFT_CONNECTED));
        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec(2 * HP - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module,
            Thru::LIGHT_RIGHT_CONNECTED));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(5.08, 40.0)), module, Thru::INPUTS_IN));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(5.08, 70.0)), module, Thru::OUTPUTS_OUT));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Thru*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(createExpandableSubmenu(module, this));
        menu->addChild(new MenuSeparator);  // NOLINT
    }
};

Model* modelThru = createModel<Thru, ThruWidget>("Thru");  // NOLINT
