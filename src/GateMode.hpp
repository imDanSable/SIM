#include "plugin.hpp"
#include "constants.hpp"

using namespace constants;
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

    Module *module;
    Modes gateMode;
    GateMode(Module *module, int paramId);
    void setGateMode(const Modes gateMode);
    /// @brief: Returns whether the gate should be on or off
    /// @param channel: The channel to check
    /// @param percentage: The value of param (0-100)
    /// @param phase: The phase of the channel (0-1)
    /// @param direction: The direction of the channel relative to last time
    /// @return: Whether the gate should be on or off
    void triggerGate(const int channel, const float percentage,const float phase, const float length, const bool direction);
    /// @brief process PulseGenerator for channel 
    /// @return returns whether the gate should be on or off
    bool process(const int channel, const float phase, const float sampleTime);
    MenuItem *createMenuItem();
private:
    int paramId;
	std::pair<float, float> relativeGate[NUM_CHANNELS] = {};
	dsp::PulseGenerator triggers[NUM_CHANNELS];

};
