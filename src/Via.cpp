#include "InX.hpp"
#include "OutX.hpp"
#include "ReX.hpp"
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

    bool readVoltages(bool forced = false)
    {
        const bool changed = this->isDirty();
        if (changed || forced) {
            readBuffer().assign(inputs[INPUTS_IN].getVoltages(),
                                inputs[INPUTS_IN].getVoltages() + inputs[INPUTS_IN].getChannels());
            refresh();
        }
        return changed;
    }

    void writeVoltages()
    {
        outputs[OUTPUT_OUT].setChannels(readBuffer().size());
        outputs[OUTPUT_OUT].writeVoltages(readBuffer().data());
    }

   public:
    Via() : Expandable({{modelReX, &this->rex}, {modelInX, &this->inx}}, {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configCache();
    }

    void performTransforms(bool forced = false)  // 100% same as Bank
    {
        bool changed = readVoltages(forced);
        bool dirtyAdapters = false;
        if (!changed && !forced) { dirtyAdapters = this->dirtyAdapters(); }
        // Update our buffer because an adapter is dirty and our buffer needs to be updated
        if (!changed && !forced && dirtyAdapters) { readVoltages(true); }

        if (changed || dirtyAdapters || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                transform(*adapter);
            }
            if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
            transform(outx);
            writeVoltages();
        }
    }

    void onUpdateExpanders(bool /*isRight*/) override
    {
        performTransforms(true);
    }

    void process(const ProcessArgs& /*args*/) override
    {
        performTransforms();
    }
};

using namespace dimensions;  // NOLINT

struct ViaWidget : public SIMWidget {
    explicit ViaWidget(Via* module)
    {
        setModule(module);
        setSIMPanel("Via");
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
        SIMWidget::appendContextMenu(menu);
        auto* module = dynamic_cast<Via*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator);
        menu->addChild(module->createExpandableSubmenu(this));
    }
};

Model* modelVia = createModel<Via, ViaWidget>("Via");  // NOLINT
