#include "Phasor.hpp"
#include <cassert>
#include <rack.hpp>
#include "Shared.hpp"

void Phasor::set(float newPhase)
{
    assert(newPhase >= 0.F && newPhase < 1.F &&
           "Phase must be normalized. For non-normalized phase, use setTotalPhase");
    prevPhase = phase;
    phase = newPhase;
};

float Phasor::get() const
{
    return phase;
};

int Phasor::getDistance() const
{
    return distance;
};

float Phasor::getTotalPhase() const
{
    return phase + distance;
};

// NOTCOVERED
void Phasor::setTotalPhase(float newTotalPhase)
{
    distance = std::floor(newTotalPhase);
    phase = newTotalPhase - distance;
    assert(phase >= 0.F && phase < 1.F &&
           "Internal error. We need to normalize phase because of FP errors. See ::process()");
};

float Phasor::getPeriod() const
{
    return timePeriod;
};

bool Phasor::isPeriodSet() const
{
    return !std::isnan(timePeriod);
};

void Phasor::reset(bool keepPeriod)
{
    phase = 0.F;
    prevPhase = 0.F;
    distance = 0;
    if (!keepPeriod) { timePeriod = NAN; }
};

// NOTCOVERED
void Phasor::process(float deltaTime)
{
    assert((std::isnan(timePeriod) == false) && "process called before setPeriod");
    const float newPhase = phase + deltaTime / timePeriod;
    const int distanceChange = std::floor(newPhase);
    if (distanceChange == 0) { phase = newPhase; }
    else {
        distance += distanceChange;
        phase = newPhase - distanceChange;
        // FP errors can cause phase to be slightly out of bounds
        if (phase < 0.F) {
            phase += 1.F;
            // distance -= 1;  // XXX Not sure about this.
        }
        else if (phase >= 1.F) {
            phase -= 1.F;
            // distance += 1;  // XXX Not sure about this.
        }
    }
};

void Phasor::setPeriod(float newTimePerPeriod)
{
    assert(!std::isnan(newTimePerPeriod) && newTimePerPeriod > 0.F &&
           "Period must be greater than 0 and not NaN");
    timePeriod = newTimePerPeriod;
};

bool Phasor::getSmartDirection() const
{
    float diff = wrap(phase - prevPhase, -0.5F, 0.5F);
    return diff > 0.F;
};

bool Phasor::getDirection() const
{
    return phase > prevPhase;
};
