
#include "plugin.hpp"

using namespace rack::engine;  // NOLINT

struct Blank : Module {
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    Blank()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    }

    void process(const ProcessArgs& args) override {}
};

struct BlankWidget : public SIMWidget {
    explicit BlankWidget(Blank* module)
    {
        setModule(module);
        setSIMPanel("Blank");
    }
};

Model* modelBlank = createModel<Blank, BlankWidget>("Blank");  // NOLINT
