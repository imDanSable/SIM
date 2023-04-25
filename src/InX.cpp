#include "InX.hpp"
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX()
    : ModuleX(
          {modelReX, modelInX}, {modelSpike, modelReX, modelInX}, [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
          [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
{
  config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

Model *modelInX = createModel<InX, InXWidget>("InX"); // NOLINT