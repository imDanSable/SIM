
#pragma once
#include <cmath>

// float wrap(float v, float lo, float hi); // from Shared.cpp
// float wrap(float v, float hi); // from Shared.cpp
// float frac(float v);

/// @brief A class that represents a phasor.
class Phasor {
   public:
    // Phase related

    /// @brief Sets the phasor's phase
    void set(float newPhase);
    /// @brief Gets the phasor's phase
    float get() const;

    // NOTCOVERED
    /// @brief Gets the phasor's number of periods passed
    int64_t getDistance() const;

    // NOTCOVERED
    /// @brief Gets the phasor's total phase, including distance
    float getTotalPhase() const;

    void setTotalPhase(float newTotalPhase);

    /// @brief Sets the phasor's time period and leaves phase untouched
    /// @param newTimePerPeriod The new time period. Must be positive and not NaN.
    void setPeriod(float newTimePerPeriod);
    float getPeriod() const;
    bool isPeriodSet() const;

    /// @brief Progresses the phasor by deltaTime
    /// @param deltaTime The time to progress the phasor by. Deltatime can be negative
    void process(float deltaTime);

    /// @brief Resets the phasor's phase and distance to 0.
    /// @param keepPeriod If true, the period is not reset.
    void reset(bool keepPeriod = false);

    bool getDirection() const;

    /// @brief Determines the direction of phase change, accounting for wrap-around.
    /// @return true if phase has increased (considering wrap-around), false otherwise.
    bool getSmartDirection() const;
    // Time related
   private:
    // static float wrapPhase(float);
    float phase = 0.F;
    // separate ints from the float for precission
    int64_t distance = 0;
    float prevPhase = 0.F;
    float timePeriod = NAN;
};