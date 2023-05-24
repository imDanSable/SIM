#pragma once
#include <iterator>
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "iters.hpp"
#include "plugin.hpp"

struct OutX : public biexpand::RightExpander {
    friend struct OutXWidget;
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { ENUMS(OUTPUT_SIGNAL, 16), OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    OutX();
    void process(const ProcessArgs& args) override;

    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;
    bool getSnoopMode() const
    {
        return snoopMode;
    }
    bool getNormalledMode() const
    {
        return normalledMode;
    }

   private:
    bool normalledMode = false;
    bool snoopMode = false;
};

class OutxAdapter : public biexpand::BaseAdapter<OutX> {
   public:
    template <typename Iter>
    /*Iter*/ void write(Iter first, Iter last, int channel = 0)
    {
        // XXX I have not checked any write return values since there was no need yet
        assert(ptr);
        assert(std::distance(first, last) < 16);
        if (ptr->getNormalledMode()) {
            // Note that argument memory of first is being used
            // if we don't the out put (visually) glitches
            auto copyFrom = first;
            for (auto& output : ptr->outputs) {
                if (output.isConnected()) {
                    int channels = std::distance(copyFrom, first) + 1;
                    output.setChannels(channels);
                    std::copy_n(copyFrom, channels,
                                iters::PortVoltageIterator(output.getVoltages()));
                    copyFrom = first + 1;
                }
                if (first == last) { return /*last*/; }
                ++first;
            }
        }
        int i = 0;
        for (auto it = first; it != last; ++it, i++) {
            if (ptr->outputs[i].isConnected()) { ptr->outputs[i].setVoltage(*it, channel); }
        }
        // return last;
    }

    template <typename InIter, typename OutIter>
    OutIter transform(InIter first, InIter last, OutIter out, int channel = 0)
    {
        assert(ptr);
        if (!ptr->getSnoopMode()) { return std::copy(first, last, out); }
        const int lastConnected = getLastConnectedIndex();
        if (!ptr->getNormalledMode() || lastConnected == -1) {  // not normalled or no connections
            auto outIt = iters::PortIterator<Output>(ptr->outputs.begin());
            auto predicate = [this, &channel, &outIt](auto& v) {
                bool exclude = outIt->isConnected();
                ++outIt;
                return !exclude;
                ;
            };
            return std::copy_if(first, last, out, predicate);
        }
        // Snoop && Normalled && at least one connection
        if (lastConnected >= std::distance(first, last)) {
            // XXX We'd like to optimize this. Now it is copying and swapping whiche is not needed
            // All inputs are covered by connected outputs
            // copy nothing
            return out;
        }
        // Copy only the inputs which are not covered by the connected outputs
        return std::copy(first + lastConnected + 1, last, out);
        
    }
    void setVoltage(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return; }
        ptr->outputs[port].setVoltage(voltage, channel);
    }
    bool setVoltageSnoop(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return false; }
        if (!ptr->outputs[port].isConnected()) { return false; }
        ptr->outputs[port].setVoltage(voltage, channel);
        return ptr->getSnoopMode();
    }
    bool isConnected(int port) const
    {
        return !(!ptr || !ptr->outputs[port].isConnected());
    }
    bool setChannels(int channels, int port) const
    {
        if (!ptr) { return false; }
        ptr->outputs[port].setChannels(channels);
        return true;
    }
    int getLastConnectedIndex() const
    {
        for (int i = constants::NUM_CHANNELS - 1; i >= 0; i--) {
            if (ptr->outputs[i].isConnected()) { return i; }
        }
        return -1;
    }

    int getFirstConnectedIndex() const
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            if (ptr->outputs[i].isConnected()) { return i; }
        }
        return -1;
    }
    bool setExclusiveOutput(int outputIndex, float value, int channel);

   private:
    /// @brief The last output index that was set to a non-zero value per channel
    std::array<int, constants::NUM_CHANNELS> lastHigh = {};
};