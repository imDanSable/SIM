#include "OutX.hpp"
#include "ModuleX.hpp"
#include "common.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

OutX::OutX()
    : ModuleX(
          {modelSpike}, {},
          [this](float value) { lights[LIGHT_LEFT_CONNECTED].setBrightness(value); },
          [this](float value) { lights[LIGHT_RIGHT_CONNECTED].setBrightness(value); })
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
}

// XXX Is this thread safe?
void OutX::process(const ProcessArgs & /*args*/)
{
    if (leftExpander.module == nullptr)
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++)
        {
            outputs[OUTPUT_SIGNAL + i].setVoltage(0.0F);
            outputs[OUTPUT_SIGNAL + i].setChannels(0);
        }
    }
}

// XXX Refactor this (and spike::process) so it doesn't return a bool
bool OutX::setExclusiveOutput(int outputIndex, float value, int channel)
{
    if (normalledMode)
    {
        outputs[lastHigh[channel]].setVoltage(0.F, channel);
        for (int i = outputIndex; i < 16; i++)
        {
            if (outputs[i].isConnected())
            {
                lastHigh[channel] = i;
                outputs[i].setVoltage(value, channel);
                return snoopMode;
            }
        }
    }
    else if (!normalledMode)
    {
        if (outputIndex != lastHigh[channel])
        {
            outputs[lastHigh[channel]].setVoltage(0.F, channel);
            lastHigh[channel] = outputIndex;
        }
        if (outputs[outputIndex].isConnected())
        {
            outputs[outputIndex].setVoltage(value, channel);
            return snoopMode;
        }
    }
    return false;
}

json_t *OutX::dataToJson()
{
    json_t *rootJ = json_object();
    json_object_set_new(rootJ, "snoopMode", json_boolean(snoopMode));
    json_object_set_new(rootJ, "normalledMode", json_boolean(normalledMode));
    return rootJ;
}
void OutX::dataFromJson(json_t *rootJ)
{
    json_t *snoopModeJ = json_object_get(rootJ, "snoopMode");
    if (snoopModeJ)
    {
        snoopMode = json_boolean_value(snoopModeJ);
    }
    json_t *normalledModeJ = json_object_get(rootJ, "normalledMode");
    if (normalledModeJ)
    {
        normalledMode = json_boolean_value(normalledModeJ);
    }
};

Model *modelOutX = createModel<OutX, OutXWidget>("OutX"); // NOLINT