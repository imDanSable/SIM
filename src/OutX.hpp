#pragma once
// SOMEDAYMAYBE: make outx multichannel aware. For now it's just 1 channel
#include <array>
#include <iterator>
#include <rack.hpp>
#include "biexpander/biexpander.hpp"
#include "constants.hpp"
#include "iters.hpp"

using namespace rack;  // NOLINT

struct OutX : public biexpand::BiExpander {
    friend struct OutXWidget;
    enum ParamId { PARAM_NORMALLED, PARAM_CUT, PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { ENUMS(OUTPUT_SIGNAL, 16), OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHTS_LEN };

    OutX();
    void process(const ProcessArgs& args) override;

    bool getCutMode() const
    {
        return params[PARAM_CUT].value > constants::BOOL_TRESHOLD;
    }
    bool getNormalledMode() const
    {
        return params[PARAM_NORMALLED].value > constants::BOOL_TRESHOLD;
    }

   private:
};

class OutxAdapter : public biexpand::BaseAdapter<OutX> {
   public:
    template <typename Iter>
    void write(Iter first, Iter last, float multiplyFactor = 1.0F)
    {
        assert(ptr);
        assert(std::distance(first, last) <= 16);
        const int inputCount = std::distance(first, last);

        if (ptr->getNormalledMode()) {
            // DOCUMENT: Normalled mode we use the channels so we have to ignore channel
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
                // if (first == last) { return /*last*/; }
                ++first;
            }
            return;
        }
        int i = 0;
        for (auto it = first; it != last; ++it, i++) {
            if (ptr->outputs[i].isConnected()) {
                ptr->outputs[i].setVoltage(*it * multiplyFactor);
                ptr->outputs[i].setChannels(1);
                // For now we do outx just 1 channel. Making it multi is easy, but for phi->outx
                // makes no sense
            }
        }
    }

    bool inPlace(int /*length*/, int /*channel*/) const override
    {
        if (!ptr) { return true; }  // Do nothing
        if (!ptr->getCutMode()) { return true; }
        bool allChannelsZero =
            std::all_of(ptr->outputs.begin(), ptr->outputs.end(),
                        [](const auto& output) { return output.channels == 0; });  // NOLINT
        return allChannelsZero;
    }

    iters::BoolIter transform(iters::BoolIter first,
                              iters::BoolIter last,
                              iters::BoolIter out,
                              int channel) const override
    {
        // When using a boolIter with cut we just false the bool and leave the length as is
        const bool normalled = ptr->getNormalledMode();
        if (!normalled) {
            auto output = iters::PortIterator<Output>(ptr->outputs.begin());
            for (auto it = first; it != last; ++it) {
                if (output->isConnected()) { *out = false; }
                else {
                    *out = *it;
                }
                ++output;
                ++out;
            };
            return out;
        }
        // Normalled and Cut (should not change the length)
        const int lastConnected =
            std::min(static_cast<int>(std::distance(first, last)) - 1, getLastConnectedIndex());
        auto current = first;
        if (lastConnected == -1) { return std::copy(first, last, out); }
        for (int i = 0; i <= lastConnected; i++) {
            *out = false;
            ++out;
            ++current;
        }
        // copy the rest
        return std::copy(current, last, out);
    }
    iters::FloatIter transform(iters::FloatIter first,
                               iters::FloatIter last,
                               iters::FloatIter out,
                               int channel) const override
    {
        return transformImpl(first, last, out, channel);
    }
    template <typename InIter, typename OutIter>
    OutIter transformImpl(InIter first, InIter last, OutIter out, int channel = 0) const
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
            auto predicate = [/*&channel, */ &outIt](auto /*v*/) {
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
        return ptr && ptr->outputs[port].isConnected();
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

    int getFirstConnectedIndex(int offset = 0) const
    {
        for (int i = offset; i < constants::NUM_CHANNELS; i++) {
            if (ptr->outputs[i].isConnected()) { return i; }
        }
        return -1;
    }

    /// @brief reports the number of connected ports. If normalled, it returns the last connected
    int totalConnected(int channel) const;

    /// @brief Writes a gate voltage to a port which might differ from port when module is normalled
    /// @return true if the gate should be cut from the original
    bool writeGateVoltage(int port, bool gateOn, int channel = 0);

   private:
    /// @brief The last output index that was set to a non-zero value per channel
    // std::array<int, constants::NUM_CHANNELS> lastHigh = {};
    std::array<int, constants::NUM_CHANNELS> lastPort = {};
};