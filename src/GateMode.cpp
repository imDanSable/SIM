#include "GateMode.hpp"
#include "plugin.hpp"

GateMode::GateMode(Module *module, int paramId)
    : module(module), gateMode(RELATIVE), paramId(paramId)
{
}

bool GateMode::process(int channel, float phase, float sampleTime)
{
    if (gateMode == RELATIVE)
    {
        if (phase >= (relativeGate[channel].first) && (phase <= (relativeGate[channel].second)))
        {
            return true;
        }
        relativeGate[channel] = std::make_pair(0.F, 0.F);
        return false;
    }
    return triggers[channel].process(sampleTime);
}

void GateMode::triggerGate(int channel, float percentage, float phase, int length, bool direction)
{

    if (gateMode == RELATIVE)
    {
        const float start = phase;
        const float delta = (1.F / length) * (percentage * 0.01F) * (direction ? 1.F : -1.F);
        relativeGate[channel] = std::minmax(start, start + delta);
    }
    else
    {
        float factor = 0.F;
        if (gateMode == ONE_TO_100MS)
        {
            factor = 0.001F;
        }
        else if (gateMode == ONE_TO_1000MS)
        {
            factor = 0.01F;
        }
        else if (gateMode == ONE_TO_10000MS)
        {
            factor = 0.1F;
        }
        const float duration = (percentage * factor);
        triggers[channel].trigger(duration < 1e-3F ? 1e-3F : duration);
    }
}

void GateMode::setGateMode(const Modes &gateMode)
{
    this->gateMode = gateMode;
    ParamQuantity *paramQuantity = module->getParamQuantity(paramId);
    switch (gateMode)
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
        paramQuantity->minValue = 0.001F;
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
    gateMode = RELATIVE;
}

MenuItem *GateMode::createMenuItem()
{
    std::vector<std::string> gateModeLabels = {"Relative", "1ms to 100ms", "1ms to 1s",
                                               "1ms to 10s"};
    return rack::createIndexSubmenuItem(
        "Gate Duration", gateModeLabels, [=]() { return gateMode; },
        [=](int index) { setGateMode(static_cast<Modes>(index)); });
}