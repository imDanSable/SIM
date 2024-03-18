

#include "AlgoX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "plugin.hpp"

struct AlgoXWdiget : ModuleWidget {
    explicit AlgoXWdiget(AlgoX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/AlgoX.svg")));
    }
};

Model* modelAlgoX = createModel<AlgoX, AlgoXWdiget>("AlgoX");  // NOLINT