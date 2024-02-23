#include "constants.hpp"
#include "plugin.hpp"

struct RelGate {
    /// @brief Returns whether the gate should be on or off (when using phase)
    /// @param channel: The channel to check
    /// @param percentage: The value of param (0-100)
    /// @param phase: The phase of the channel (0-1)
    /// @param direction: The direction of the channel relative to last time
    void triggerGate(int channel, float percentage, float phase, int length, bool direction);

    /// @brief Returns whether the gate should be on or off (when using trigger)
    /// @param channel: The channel to check
    /// @param percentage: The value of param (0-100)
    void triggerGate(int channel, float percentage, float samples_per_pulse);

    /// @brief process PulseGenerator for channel
    /// @return returns whether the gate should be on or off
    bool processPhase(int channel, float phase);
    bool processTrigger(int channel, float sampleTime);
    void reset();

   private:
    std::array<std::pair<float, float>, constants::NUM_CHANNELS> gateWindow = {};
    std::array<dsp::PulseGenerator, constants::NUM_CHANNELS> pulseGenerator = {};
};
