#include "RelGate.hpp"
#include "constants.hpp"
#include "plugin.hpp"

bool RelGate::processPhase(int channel, float phase)
{
    if (phase >= (gateWindow[channel].first) && (phase <= (gateWindow[channel].second))) {
        return true;
    }
    gateWindow[channel] = std::make_pair(0.F, 0.F);
    return false;
}

bool RelGate::processTrigger(int channel, float sampleTime)
{
    return pulseGenerator[channel].process(sampleTime);
}

void RelGate::triggerGate(int channel, float percentage, float phase, int length, bool direction)
{
    const float start = phase;
    const float delta = (1.F / length) * (percentage * 0.01F) * (direction ? 1.F : -1.F);
    gateWindow[channel] = std::minmax(start, start + delta);
}

void RelGate::triggerGate(int channel, float percentage, float samples_per_pulse)
{
    pulseGenerator[channel].trigger(
        clamp(percentage * .01F * samples_per_pulse, 1e-3F, samples_per_pulse));
}

void RelGate::reset()
{
    gateWindow = {};
    pulseGenerator = {};
}