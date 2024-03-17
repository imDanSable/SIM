#include <array>
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
struct Via : biexpand::Expandable {
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_IN, INPUTS_LEN };
    enum OutputId { OUTPUT_OUT, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct ViaWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;

    std::vector<float> v1, v2;
    std::array<std::vector<float>*, 2> voltages{&v1, &v2};
    std::vector<float>& readBuffer()
    {
        return *voltages[0];
    }
    std::vector<float>& writeBuffer()
    {
        return *voltages[1];
    }
    void swap()
    {
        std::swap(voltages[0], voltages[1]);
    }

    void readVoltages()
    {
        auto& input = inputs[INPUTS_IN];
        std::copy_n(iters::PortVoltageIterator(input.getVoltages()), input.getChannels(),
                    voltages[0]->begin());
        voltages[0]->resize(input.getChannels());
    }

    void writeVoltages()
    {
        outputs[OUTPUT_OUT].channels = voltages[0]->size();  // NOLINT

        outputs[OUTPUT_OUT].writeVoltages(voltages[0]->data());
    }

   public:
    Via() : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        v1.resize(constants::NUM_CHANNELS);
        v2.resize(constants::NUM_CHANNELS);
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    template <typename Adapter>  // Double
    void perform_transform(Adapter& adapter)
    {
        if (adapter) {
            writeBuffer().resize(16);
            auto newEnd = adapter.transform(readBuffer().begin(), readBuffer().end(),
                                            writeBuffer().begin(), 0);
            const int channels = std::distance(writeBuffer().begin(), newEnd);
            assert((channels <= 16) && (channels >= 0));  // NOLINT
            writeBuffer().resize(channels);
            swap();
        }
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
        //  for (auto adapter = getLeftAdapters().rbegin(); adapter != getLeftAdapters().rend();
        //  ++adapter) {
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
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Via.svg")));
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
