#include "RelGate.hpp"

#include <cmath>
#include "constants.hpp"
#include "plugin.hpp"

bool RelGate::processPhase(int channel, float phase)
{
    const float start = gateWindow[channel].first;
    const float end = std::fmod(start + gateWindow[channel].second, 1.F);
    bool retVal = false;
    if (start < end) { retVal = (start <= phase) && (phase < end); }
    else {
        retVal = (start <= phase) || (phase < end);
    }
    if (!retVal) {
        gateWindow[channel] = std::make_pair(NAN, NAN);
    }
    return retVal;
}

bool RelGate::processTrigger(int channel, float sampleTime)
{
    return pulseGenerator[channel].process(sampleTime);
}

void RelGate::triggerGate(int channel, float percentage, float phase, int length, bool direction)
{
    float start = phase;
    const float delta = std::abs(1.F / length) * (percentage * 0.01F);
    if (!direction) { start = std::fmod(start - delta + 1.F, 1.F); }
    gateWindow[channel] = {start, delta};
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