

#include "AlgoX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "plugin.hpp"

struct AlgoXWdiget : ModuleWidget {
    explicit AlgoXWdiget(AlgoX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/AlgoX.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/AlgoX.svg")));
    }
};

Model* modelAlgoX = createModel<AlgoX, AlgoXWdiget>("AlgoX");  // NOLINT