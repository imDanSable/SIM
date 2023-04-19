#include "GateMode.hpp"
#include "plugin.hpp"

GateMode::GateMode(Module *module, int paramId) : module(module), gateMode(RELATIVE),  paramId(paramId)
{

}

//XXX refactor: This function serves two purposes.
bool GateMode::process(const int channel, const float phase, const float sampleTime/*, const float phaseSpeed*/)
{
    if (gateMode == RELATIVE)
    {
        // Pre-empt 
        // const float topDelta = phaseSpeed > 0.f ? phaseSpeed : 0.f;
        // const float bottomDelta = phaseSpeed < 0.f ? phaseSpeed : 0.f;
        // const float topDelta = 0;
        // const float bottomDelta = 0;

        if (phase >= (relativeGate[channel].first /*- bottomDelta*/) && (phase <= (relativeGate[channel].second /*- topDelta */)))
        {
            return true;
        }
        else
        {
            relativeGate[channel] = std::make_pair(0.f, 0.f);
            return false;
        }
    }
    else
    {
        return triggers[channel].process(sampleTime);
    }
    return false;
}

void GateMode::triggerGate(const int channel, const float percentage, const float phase, const float length, const bool direction)
{

    if (gateMode == RELATIVE)
    {
        const float start = phase;
        const float delta = (1.f / length) * (percentage * 0.01f) * (direction ? 1.f : -1.f);
        relativeGate[channel] = std::minmax(start, start + delta);
    }
    else
    {
        float factor = 0.f;
        if (gateMode == ONE_TO_100MS)
            factor = 0.001f;
        else if (gateMode == ONE_TO_1000MS)
            factor = 0.01f;
        else if (gateMode == ONE_TO_10000MS)
            factor = 0.1f;
        const float duration = (percentage * factor);
        triggers[channel].trigger(duration < 1e-3f ? 1e-3f : duration);
    }
}

void GateMode::setGateMode(const Modes gateMode)
{
    this->gateMode = gateMode;
    ParamQuantity *paramQuantity = module->getParamQuantity(paramId);
    switch (gateMode)
    {
    case RELATIVE:
        paramQuantity->minValue = 1.f;
        paramQuantity->maxValue = 100.f;
        paramQuantity->displayMultiplier = 1.f;
        paramQuantity->defaultValue = 1.f;
        paramQuantity->unit = "%";
        break;
    case ONE_TO_100MS:
        paramQuantity->minValue = 1.f;
        paramQuantity->maxValue = 100.f;
        paramQuantity->displayMultiplier = 1.f;
        paramQuantity->defaultValue = 1.f;
        paramQuantity->unit = "ms";
        break;
    case ONE_TO_1000MS:
        paramQuantity->minValue = 1.f;
        paramQuantity->maxValue = 100.f;
        paramQuantity->displayMultiplier = 10.f;
        paramQuantity->defaultValue = 1.f;
        paramQuantity->unit = "ms";
        break;
    case ONE_TO_10000MS:
        paramQuantity->minValue = 0.001f;
        paramQuantity->maxValue = 100.f;
        paramQuantity->displayMultiplier = 0.1f;
        paramQuantity->defaultValue = 0.001f;
        paramQuantity->unit = "s";
        break;
    default:
        break;
    }
}

MenuItem *GateMode::createMenuItem()
{
    std::vector<std::string> gateModeLabels = {"Relative", "1ms to 100ms", "1ms to 1s", "1ms to 10s"};
    return rack::createIndexSubmenuItem(
        "Gate Duration",
        gateModeLabels,
        [=]()
        { return gateMode; },
        [=](int index)
        { setGateMode((Modes)index); });
}