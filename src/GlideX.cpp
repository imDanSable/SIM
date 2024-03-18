

#include "GlideX.hpp"
#include "biexpander/biexpander.hpp"
#include "common.hpp"
#include "plugin.hpp"

struct GlideXWdiget : ModuleWidget {
    explicit GlideXWdiget(GlideX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/GlideX.svg")));
    }
};

Model* modelGlideX = createModel<GlideX, GlideXWdiget>("GlideX");  // NOLINT