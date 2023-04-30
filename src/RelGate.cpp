#include "RelGate.hpp"
#include "constants.hpp"
#include "plugin.hpp"

bool RelGate::process(int channel, float phase)
{
    if (phase >= (gateWindow[channel].first) && (phase <= (gateWindow[channel].second))) {
        return true;
    }
    gateWindow[channel] = std::make_pair(0.F, 0.F);
    return false;
}

void RelGate::triggerGate(int channel, float percentage, float phase, int length, bool direction)
{
    const float start = phase;
    const float delta = (1.F / length) * (percentage * 0.01F) * (direction ? 1.F : -1.F);
    gateWindow[channel] = std::minmax(start, start + delta);
}

void RelGate::reset()
{
    gateWindow = {};
}