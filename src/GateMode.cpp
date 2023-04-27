#include "GateMode.hpp"
#include "constants.hpp"
#include "dsp/digital.hpp"
#include "helpers.hpp"
#include "plugin.hpp"
#include "ui/MenuItem.hpp"

GateMode::GateMode(Module *module, int paramId)
    : module(module), gateDuration(RELATIVE), paramId(paramId)
{
}

bool GateMode::process(int channel, float phase, float sampleTime /* , int maxGates */)
{
    if (gateDuration == RELATIVE)
    {
        if (phase >= (relativeGate[channel].first) && (phase <= (relativeGate[channel].second)))
        {
            return true;
        }
        relativeGate[channel] = std::make_pair(0.F, 0.F);
        return false;
    }
    if (exclusiveGates)
    {
        // No need to reset triggers outside 'channel'
        return triggers[channel].process(sampleTime);
    }
    bool result = false;
    for (int i = 0; i < constants::MAX_GATES; i++)
    {
        result |= triggers[i].process(sampleTime);
    }
    return result;
}

void GateMode::triggerGate(int channel, float percentage, float phase, int length, bool direction)
{

    if (gateDuration == RELATIVE)
    {
        const float start = phase;
        const float delta = (1.F / length) * (percentage * 0.01F) * (direction ? 1.F : -1.F);
        relativeGate[channel] = std::minmax(start, start + delta);
        return;
    }
    float factor = 0.F;
    if (gateDuration == ONE_TO_100MS)
    {
        factor = 0.001F;
    }
    else if (gateDuration == ONE_TO_1000MS)
    {
        factor = 0.01F;
    }
    else if (gateDuration == ONE_TO_10000MS)
    {
        factor = 0.1F;
    }
    const float duration = (percentage * factor);
    triggers[channel].trigger(duration < 1e-3F ? 1e-3F : duration);
}

bool GateMode::getExclusiveGates() const
{
    return exclusiveGates;
}

void GateMode::setExclusiveGates(bool exclusiveGates)
{
    this->exclusiveGates = exclusiveGates;
}

GateMode::Duration GateMode::getGateDuration() const
{
    return gateDuration;
}

void GateMode::setGateDuration(const Duration &gateDuration)
{
    this->gateDuration = gateDuration;
    ParamQuantity *paramQuantity = module->getParamQuantity(paramId);
    switch (gateDuration)
    {
    case RELATIVE:
        paramQuantity->minValue = 1.F;
        paramQuantity->maxValue = 100.F;
        paramQuantity->displayMultiplier = 1.F;
        paramQuantity->defaultValue = 1.F;
        paramQuantity->unit = "%";
        break;
    case ONE_TO_100MS:
        paramQuantity->minValue = 1.F;
        paramQuantity->maxValue = 100.F;
        paramQuantity->displayMultiplier = 1.F;
        paramQuantity->defaultValue = 1.F;
        paramQuantity->unit = "ms";
        break;
    case ONE_TO_1000MS:
        paramQuantity->minValue = 1.F;
        paramQuantity->maxValue = 100.F;
        paramQuantity->displayMultiplier = 10.F;
        paramQuantity->defaultValue = 1.F;
        paramQuantity->unit = "ms";
        break;
    case ONE_TO_10000MS:
        paramQuantity->minValue = 1.F;
        paramQuantity->maxValue = 100.F;
        paramQuantity->displayMultiplier = 0.1F;
        paramQuantity->defaultValue = 0.001F;
        paramQuantity->unit = "s";
        break;
    default:
        break;
    }
}

void GateMode::reset()
{
    for (auto &trigger : triggers)
    {
        trigger.reset();
    }
    gateDuration = RELATIVE;
}

MenuItem *GateMode::createGateDurationItem()
{
    std::vector<std::string> gateDurationLabels = {"Relative", "1ms to 100ms", "10ms to 1s",
                                                   "100ms to 10s"};
    return rack::createIndexSubmenuItem(
        "Gate Duration", gateDurationLabels, [=]() { return gateDuration; },
        [=](int index) { setGateDuration(static_cast<Duration>(index)); });
}

MenuItem *GateMode::createExclusiveMenuItem()
{
    return rack::createBoolPtrMenuItem("Exclusive Gates", "", &exclusiveGates);
}