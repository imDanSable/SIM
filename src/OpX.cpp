

#include "OpX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "plugin.hpp"

struct OpXWdiget : ModuleWidget {
    explicit OpXWdiget(OpX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/OpX.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/OpX.svg")));
    }
};

Model* modelOpX = createModel<OpX, OpXWdiget>("OpX");  // NOLINT