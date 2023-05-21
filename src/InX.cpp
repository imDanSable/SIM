#include "InX.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX()
     // : ModuleX(false,
                              //           LIGHT_LEFT_CONNECTED,
                              //           LIGHT_RIGHT_CONNECTED)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

Model* modelInX = createModel<InX, InXWidget>("InX");  // NOLINT