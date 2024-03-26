

#include "OpX.hpp"
#include "plugin.hpp"

struct OpXWdiget : public SIMWidget {
    explicit OpXWdiget(OpX* module)
    {
        setModule(module);
        setSIMPanel("OpX");
    }
};

Model* modelOpX = createModel<OpX, OpXWdiget>("OpX");  // NOLINT