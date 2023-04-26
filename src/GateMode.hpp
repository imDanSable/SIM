#include "constants.hpp"
#include "plugin.hpp"

struct GateMode
{
    enum Modes
    {
        RELATIVE,
        ONE_TO_100MS,
        ONE_TO_1000MS,
        ONE_TO_10000MS,
        NUM_GATE_MODES
    };

    GateMode(Module *module, int paramId);
    void setGateMode(const Modes &gateMode);
    Modes getGateMode() const
    {
        return gateMode;
    }
    /// @brief: Returns whether the gate should be on or off
    /// @param channel: The channel to check
    /// @param percentage: The value of param (0-100)
    /// @param phase: The phase of the channel (0-1)
    /// @param direction: The direction of the channel relative to last time
    void triggerGate(int channel, float percentage, float phase, int length, bool direction);
    /// @brief process PulseGenerator for channel
    /// @return returns whether the gate should be on or off
    bool process(int channel, float phase, float sampleTime);
    void reset();
    MenuItem *createMenuItem();

  private:
    Module *module;
    Modes gateMode;
    int paramId;
    std::array<std::pair<float, float>, constants::NUM_CHANNELS> relativeGate = {};
    std::array<dsp::PulseGenerator, constants::NUM_CHANNELS> triggers = {};
};
