#pragma once
#include <array>
#include <iterator>
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "iters.hpp"
#include "plugin.hpp"

struct OutX : public biexpand::RightExpander {
    friend struct OutXWidget;
    enum ParamId { PARAM_NORMALLED, PARAM_CUT, PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { ENUMS(OUTPUT_SIGNAL, 16), OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    OutX();
    void process(const ProcessArgs& args) override;

    bool getCutMode() const
    {
        return params[PARAM_CUT].value > 0.5;
    }
    bool getNormalledMode() const
    {
        return params[PARAM_NORMALLED].value > 0.5;
    }

   private:
};

class OutxAdapter : public biexpand::BaseAdapter<OutX> {
   public:
    template <typename Iter>
    void write(Iter first, Iter last, int channel = 0)
    {
        assert(ptr);
        assert(std::distance(first, last) <= 16);
        const int inputCount = std::distance(first, last);
        const int lastConnected = getLastConnectedIndex();
        int portIndex = 0;

        if (ptr->getNormalledMode()) {

            // Note that argument memory of first is being used
            // if we don't the output (visually) glitches
            auto copyFrom = first;
            for (auto& output : ptr->outputs) {
                if (output.isConnected()) {
                    int channels = std::distance(copyFrom, first) + 1;
                    output.setChannels(clamp(channels, 1, inputCount));
                    // output.channels = channels;
                    std::copy_n(copyFrom, channels,
                                iters::PortVoltageIterator(output.getVoltages()));
                    copyFrom = first + 1;
                }
                if (first == last) { return /*last*/; }
                ++first;
                portIndex++;
            }
        }
        int i = 0;
        for (auto it = first; it != last; ++it, i++) {
            if (ptr->outputs[i].isConnected()) { ptr->outputs[i].setVoltage(*it, channel); }
        }
        // return last;
    }

    iters::BufIter transform(iters::BufIter first,
                             iters::BufIter last,
                             iters::BufIter out,
                             int channel = 0) override
    {
        assert(ptr);
        const bool normalled = ptr->getNormalledMode();
        const bool cut = ptr->getCutMode();
        // No Cutting, just copy
        if (!cut) { return std::copy(first, last, out); }
        const int lastConnected = getLastConnectedIndex();
        // No outx connections, just copy
        if (lastConnected == -1) { return std::copy(first, last, out); }

        // Not normalled and cut. Itereate and leave out the connections
        if (!normalled) {  // not normalled
            auto outIt = iters::PortIterator<Output>(ptr->outputs.begin());
            auto predicate = [this, &channel, &outIt](auto& /*v*/) {
                bool exclude = outIt->isConnected();
                ++outIt;
                return !exclude;
                ;
            };
            return std::copy_if(first, last, out, predicate);
        }
        // Cut && Normalled && at least one connection
        if (lastConnected >= std::distance(first, last)) {
            // All inputs are covered by connected outputs
            // copy nothing
            return out;
        }
        // Copy only the inputs which are not covered by the connected outputs
        // So everything beyond lastconnected
        return std::copy(first + lastConnected + 1, last, out);
    }
    void setVoltage(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return; }
        ptr->outputs[port].setVoltage(voltage, channel);
    }
    bool setVoltageCut(float voltage, int port, int channel = 0)
    {
        if (!ptr) { return false; }
        if (!ptr->outputs[port].isConnected()) { return false; }
        ptr->outputs[port].setVoltage(voltage, channel);
        return ptr->getCutMode();
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
    bool setPortVoltage(int outputIndex, float value, int channel);

   private:
    /// @brief The last output index that was set to a non-zero value per channel
    std::array<int, constants::NUM_CHANNELS> lastHigh = {};
};