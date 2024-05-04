#include "ClockTracker.hpp"

namespace sp {

void ClockTracker::init(float avgPeriod)
{
    triggersPassed = 0;
    if (!std::isnan(avgPeriod) && avgPeriod > 0.0F) {
        this->avgPeriod = avgPeriod;
        periodDetected = true;
    }
    else {
        periodDetected = false;
    }
    timePassed = 0.0F;
}

float ClockTracker::getPeriod() const
{
    return avgPeriod;
}

bool ClockTracker::isPeriodDetected() const
{
    return periodDetected;
}

float ClockTracker::getTimePassed() const
{
    return timePassed;
}

float ClockTracker::getTimeFraction() const
{
    return timePassed / avgPeriod;
}

bool ClockTracker::process(const float dt, const float pulse)
{
    timePassed += dt;
    if (!clockTrigger.process(pulse)) { return false; }
    if (triggersPassed < 3) { triggersPassed += 1; }
    if (triggersPassed > 2) {
        periodDetected = true;
        avgPeriod = timePassed;
    }
    timePassed = 0.0F;
    return true;
};

}  // namespace sp