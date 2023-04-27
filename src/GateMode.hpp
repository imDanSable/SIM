#include "constants.hpp"
#include "plugin.hpp"

struct GateMode
{
    enum Duration
    {
        RELATIVE,
        ONE_TO_100MS,
        ONE_TO_1000MS,
        ONE_TO_10000MS,
        NUM_GATE_MODES
    };

    GateMode(Module *module, int paramId);
    void setGateDuration(const Duration &gateDuration);
    Duration getGateDuration() const;
    void setExclusiveGates(bool exclusiveGates);
    bool getExclusiveGates() const;
    /// @brief: Returns whether the gate should be on or off
    /// @param channel: The channel to check
    /// @param percentage: The value of param (0-100)
    /// @param phase: The phase of the channel (0-1)
    /// @param direction: The direction of the channel relative to last time
    void triggerGate(int channel, float percentage, float phase, int length, bool direction);
    /// @brief process PulseGenerator for channel
    /// @return returns whether the gate should be on or off
    bool process(int channel, float phase, float sampleTime /* , int maxGates */);
    void reset();
    MenuItem *createGateDurationItem();
    MenuItem *createExclusiveMenuItem();

  private:
    Module *module;
    Duration gateDuration;
    bool exclusiveGates = false;
    int paramId;
    std::array<std::pair<float, float>, constants::NUM_CHANNELS> relativeGate = {};
    std::array<dsp::PulseGenerator, constants::NUM_CHANNELS> triggers = {};
};
