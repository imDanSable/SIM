#include "Rex.hpp"
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ReX::ReX()
    : ModuleX(
          {modelInX}, {modelInX, modelSpike, modelArray},
          [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
          [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(PARAM_START, 0.0F, 15.0F, 0.0F, "Start", "", 0.0F, 1.0F, 1.0F);
    configParam(PARAM_LENGTH, 1.0F, 16.0F, 16.0F, "Length");

    getParamQuantity(PARAM_START)->snapEnabled = true;
    getParamQuantity(PARAM_LENGTH)->snapEnabled = true;
    configInput(INPUT_START, "Start CV");
    configInput(INPUT_LENGTH, "Length CV");
};

int ReX::getStart(int channel)
{
    if (inputs[INPUT_START].isConnected())
    {
        return inputs[INPUT_START].getVoltage(channel);
    }
    return params[PARAM_START].getValue();
}

int ReX::getLength(int channel)
{
    if (inputs[INPUT_LENGTH].isConnected())
    {
        return inputs[INPUT_LENGTH].getVoltage(channel);
    }
    return params[PARAM_LENGTH].getValue();
}

Model *modelReX = createModel<ReX, ReXWidget>("ReX"); // NOLINT
