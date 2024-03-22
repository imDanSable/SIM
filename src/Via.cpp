#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using InputIterator = iters::PortIterator<rack::engine::Input>;
using OutputIterator = iters::PortIterator<rack::engine::Output>;
struct Via : biexpand::Expandable<float> {
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_IN, INPUTS_LEN };
    enum OutputId { OUTPUT_OUT, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct ViaWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;

    void readVoltages()
    {
        auto& input = inputs[INPUTS_IN];
        std::copy_n(iters::PortVoltageIterator(input.getVoltages()), input.getChannels(),
                    readBuffer().begin());
        readBuffer().resize(input.getChannels());
    }

    void writeVoltages()
    {
        outputs[OUTPUT_OUT].channels = readBuffer().size();  // NOLINT

        outputs[OUTPUT_OUT].writeVoltages(readBuffer().data());
    }

   public:
    Via() : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        // v1.resize(constants::NUM_CHANNELS);
        // v2.resize(constants::NUM_CHANNELS);
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void performTransforms()  // Double
    {
        readVoltages();
        for (biexpand::Adapter* adapter : getLeftAdapters()) {
            perform_transform(*adapter);
        }
        if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
        perform_transform(outx);
        writeVoltages();
    }
    void onUpdateExpanders(bool /*isRight*/) override
    {
        performTransforms();
    }

    void process(const ProcessArgs& /*args*/) override
    {
        readVoltages();
        for (biexpand::Adapter* adapter : getLeftAdapters()) {
            perform_transform(*adapter);
        }
        if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
        perform_transform(outx);
        writeVoltages();
    }
};

using namespace dimensions;  // NOLINT

struct ViaWidget : ModuleWidget {
    explicit ViaWidget(Via* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/Via.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/Via.svg")));
        if (module) {
            module->addDefaultConnectionLights(this, Via::LIGHT_LEFT_CONNECTED,
                                               Via::LIGHT_RIGHT_CONNECTED);
        }

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, JACKYSTART)), module, Via::INPUTS_IN));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, JACKYSTART + 7 * JACKYSPACE)), module,
                                                Via::OUTPUT_OUT));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Via*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
    }
};

Model* modelVia = createModel<Via, ViaWidget>("Via");  // NOLINT
