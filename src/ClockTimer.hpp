#include <optional>
#include <rack.hpp>
#include "Phasor.hpp"

/// @brief A class that tracks the time between clock triggers.
class ClockTimer {
   public:
    explicit ClockTimer(float defaultPeriod = NAN);

    /// @brief Initializes the clock timer.
    /// @param keepPeriod If true, the period will be kept, otherwise it will be reset to the
    void init(bool keepPeriod = false);
    /// @brief Advances the clock
    /// @param deltaTime The time elapsed since the last call to this function.
    /// @param cvValue The value of the driving clock signal.
    /// @param resetValue The value of the reset signal.
    /// @return true if the clock has triggered, false otherwise.
    bool process(float deltaTime, float cvValue);

    /// @brief Manually sets the time between clock triggers.
    /// @param newPeriod The new time between clock triggers.
    void setPeriod(float newPeriod);
    /// @brief Returns the time between the last two clock triggers.
    /// @return The time between the last two clock triggers.
    std::optional<float> getPeriod() const;

    /// @brief Resets the clock timer.
    /// @param keepPeriod If true, the period will be kept, otherwise it will be reset.

   private:
    rack::dsp::SchmittTrigger clockTrigger;
    rack::dsp::SchmittTrigger resetTrigger;
    rack::dsp::PulseGenerator resetGate;

    float period = 0.5F;
    float timeSinceLastClock = 0.F;
    unsigned int pulsesSinceLastReset = 0;
};

/// NOTCOVERED
class ClockPhasor {
   public:
    explicit ClockPhasor(float defaultPeriod = NAN);
    bool process(float sampleTime, float cv);
    float getPhase() const;
    void setPhase(float newPhase);
    void setPeriod(float newPeriod);
    float getPeriod() const;
    bool isPeriodSet() const;
    void setPeriodFactor(float newPeriodFactor);
    float getPeriodFactor() const;
    void reset(bool keepPeriod = false);
    bool getDirection() const;
    bool getSmartDirection() const;

   private:
    Phasor phasor;
    ClockTimer clockTimer;
    float periodFactor = NAN;
};
