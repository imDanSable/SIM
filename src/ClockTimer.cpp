#include "ClockTimer.hpp"

ClockTimer::ClockTimer(float defaultPeriod) : period(defaultPeriod)
{
    if (!std::isnan(defaultPeriod)) { pulsesSinceLastReset = 2; }
}

void ClockTimer::init(bool keepPeriod)
{
    timeSinceLastClock = 0.F;
    pulsesSinceLastReset = keepPeriod ? pulsesSinceLastReset : 0;
    period = keepPeriod ? period : 0.5F;
    resetTrigger.reset();
}

bool ClockTimer::process(float deltaTime, float cvValue)
{
    timeSinceLastClock += deltaTime;
    if (!clockTrigger.process(cvValue)) { return false; };
    if (pulsesSinceLastReset < 2) { pulsesSinceLastReset++; }
    if (pulsesSinceLastReset > 1) { period = timeSinceLastClock; }
    timeSinceLastClock = 0.F;
    return true;
}

void ClockTimer::setPeriod(float newPeriod)
{
    period = newPeriod;
    pulsesSinceLastReset = 2;
}

std::optional<float> ClockTimer::getPeriod() const
{
    if (pulsesSinceLastReset > 1) { return period; }
    return std::nullopt;
}

ClockPhasor::ClockPhasor(float defaultPeriod) : clockTimer(defaultPeriod)
{
    if (!std::isnan(defaultPeriod)) { phasor.setPeriod(defaultPeriod); }
}

bool ClockPhasor::process(float sampleTime, float cv)
{
    const bool triggered = clockTimer.process(sampleTime, cv);
    if (clockTimer.getPeriod().has_value()) {
        if (!std::isnan(periodFactor)) {
            phasor.setPeriod(clockTimer.getPeriod().value() * periodFactor);
        }
        else {
            phasor.setPeriod(clockTimer.getPeriod().value());
        }
        phasor.process(sampleTime);
    }
    return triggered;
}

float ClockPhasor::getPhase() const
{
    return phasor.get();
}

void ClockPhasor::setPhase(float newPhase)
{
    phasor.set(newPhase);
}

void ClockPhasor::setPeriod(float newPeriod)
{
    clockTimer.setPeriod(newPeriod);
    phasor.setPeriod(newPeriod);
}

float ClockPhasor::getPeriod() const
{
    return clockTimer.getPeriod().value_or(NAN);
}

bool ClockPhasor::isPeriodSet() const
{
    return clockTimer.getPeriod().has_value();
}

void ClockPhasor::setPeriodFactor(float newPeriodFactor)
{
    assert(newPeriodFactor > 0.F && "Time scale must be greater than 0");
    periodFactor = newPeriodFactor;
}

float ClockPhasor::getPeriodFactor() const
{
    return periodFactor;
}

void ClockPhasor::reset(bool keepPeriod)
{
    phasor.reset();
    clockTimer.init(keepPeriod);
}

bool ClockPhasor::getDirection() const
{
    return phasor.getDirection();
}

bool ClockPhasor::getSmartDirection() const
{
    return phasor.getSmartDirection();
}