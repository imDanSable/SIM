

#include "AlgoX.hpp"
#include "plugin.hpp"

struct AlgoXWdiget : public SIMWidget {
    explicit AlgoXWdiget(AlgoX* module)
    {
        setModule(module);
        setSIMPanel("AlgoX");
    }
};

Model* modelAlgoX = createModel<AlgoX, AlgoXWdiget>("AlgoX");  // NOLINT