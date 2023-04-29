#include "Rex.hpp"
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ReX::ReX()
    : ModuleX(false,
              {modelInX},
              {modelInX, modelSpike, modelArray, modelThru},
              LIGHT_LEFT_CONNECTED,
              LIGHT_RIGHT_CONNECTED)
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(PARAM_START, 0.0F, 15.0F, 0.0F, "Start", "", 0.0F, 1.0F, 1.0F);
    configParam(PARAM_LENGTH, 1.0F, 16.0F, 16.0F, "Length");

    getParamQuantity(PARAM_START)->snapEnabled = true;
    getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
    configInput(INPUT_START, "Start CV");
    configInput(INPUT_LENGTH, "Length CV");
};

Model* modelReX = createModel<ReX, ReXWidget>("ReX");  // NOLINT
