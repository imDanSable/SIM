#include "InX.hpp"
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX()
    : ModuleX({modelReX, modelInX},
              {modelSpike, modelReX, modelInX, modelThru},
              LIGHT_LEFT_CONNECTED,
              LIGHT_RIGHT_CONNECTED)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

int InX::getFirstConnectedInputIndex()
{
    for (int i = 0; i < constants::NUM_CHANNELS; i++) {
        if (inputs[INPUT_SIGNAL + i].isConnected()) { return i; }
    }
    return -1;
}

int InX::getLastConnectedInputIndex()
{
    for (int i = constants::NUM_CHANNELS - 1; i >= 0; i--) {
        if (inputs[INPUT_SIGNAL + i].isConnected()) { return i; }
    }
    return -1;
}

Model* modelInX = createModel<InX, InXWidget>("InX");  // NOLINT