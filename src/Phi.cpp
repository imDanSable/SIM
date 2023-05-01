#include "Expandable.hpp"
#include "plugin.hpp"

class Phi : public Expandable {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct PhiWidget;

   public:
    Phi()
        : Expandable({modelReX, modelInX}, {modelOutX}, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void process(const ProcessArgs& args) override {}
};

struct PhiWidget : ModuleWidget {
    explicit PhiWidget(Phi* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Phi.svg")));

        if (!module) { return; }
        module->addConnectionLights(this);
    };
};

Model* modelPhi = createModel<Phi, PhiWidget>("Phi");  // NOLINT
