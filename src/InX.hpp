#pragma once
#include <cassert>
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct InX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAM_INSERTMODE, PARAMS_LEN };
    enum InputId { ENUMS(INPUT_SIGNAL, 16), INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    InX();
    inline bool getInsertMode() const
    {
        return params[PARAM_INSERTMODE].value > 0.5;
    }

   private:
    friend struct InXWidget;
};

// Import BufIter from iters so we can use it
class InxAdapter : public biexpand::BaseAdapter<InX> {
   public:
    iters::BufIter transform(iters::BufIter first, iters::BufIter last, iters::BufIter out, int channel = 0) override
    {
        // std::copy(first, last, out);
        // return out;
        assert(ptr);
        bool connected = false;
        int channel_counter = 0;

        if (ptr->getInsertMode()) {
            int lastConnectedInputIndex = getLastConnectedInputIndex();
            if (lastConnectedInputIndex == -1) { return std::copy(first, last, out); }
            auto input = first;
            // Loop over inx ports
            for (int inx_port = 0; (inx_port < lastConnectedInputIndex + 1) &&
                                   (channel_counter < constants::NUM_CHANNELS);
                 ++inx_port) {
                if (ptr->inputs[inx_port].isConnected()) {
                    // Loop over inx.port channels
                    for (int port_channel = 0; port_channel < ptr->inputs[inx_port].getChannels();
                         ++port_channel) {
                        *out = ptr->inputs[inx_port].getPolyVoltage(port_channel);
                        ++channel_counter;
                        ++out;
                        if (channel_counter == constants::NUM_CHANNELS) {
                            return out;
                        }  // Maximum number of channels reached
                    }
                }
                // if There's are still items in the input, copy one
                if (input != last) {
                    *out = *input;
                    ++input;
                    ++out;
                    ++channel_counter;
                }
            }
            // Copy the rest of the input using std::copy keeping an eye on the channel counter
            while (input != last && channel_counter < constants::NUM_CHANNELS) {
                *out = *input;
                ++input;
                ++out;
                ++channel_counter;
            }
            return out;
        }

        // Not Insertmode
        int i = 0;
        for (auto it = first; it != last && i < 16; ++it, ++out, ++i) {
            connected = ptr->inputs[i].isConnected();
            *out = connected ? ptr->inputs[i].getVoltage(channel) : *it;
            if (ptr->getInsertMode() && connected) { --it; }
        }
        return out;
    }

    bool getInsertMode() const
    {
        if (!ptr) { return false; }
        return ptr->getInsertMode();
    }

    int getConnectionCount(int upTo = 16) const
    {
        if (!ptr) { return 0; }
        int count = 0;
        for (int i = 0; i < upTo; i++) {
            if (ptr->inputs[i].isConnected()) { count++; }
        }
        return count;
    }

    int getSummedChannels(int from = 0, int upTo = 16) const
    {
        if (!ptr) { return 0; }
        int count = 0;
        for (int i = from; i < upTo; i++) {
            if (ptr->inputs[i].isConnected()) { count += ptr->inputs[i].getChannels(); }
        }
        return count;
    }

    float getVoltage(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getVoltage();
    }
    float getNormalVoltage(int normalVoltage, int port) const
    {
        if (!ptr) { return normalVoltage; }
        return ptr->inputs[port].getNormalVoltage(normalVoltage);
    }
    float getPolyVoltage(int channel, int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getPolyVoltage(channel);
    }

    float getNormalPolyVoltage(float normalVoltage, int channel, int port)
    {
        if (!ptr || !ptr->inputs[port].isConnected()) { return normalVoltage; }
        return ptr->inputs[port].getPolyVoltage(channel);
    }
    bool isConnected(int port) const
    {
        return !(!ptr || !ptr->inputs[port].isConnected());
    }
    int getChannels(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getChannels();
    }
    int getNormalChannels(int normalChannels, int port) const
    {
        if (!ptr) { return normalChannels; }
        return ptr->inputs[port].getChannels();
    }
    int getLastConnectedInputIndex() const
    {
        for (int i = constants::NUM_CHANNELS - 1; i >= 0; i--) {
            if (ptr->inputs[i].isConnected()) { return i; }
        }
        return -1;
    }

    int getFirstConnectedInputIndex() const
    {
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            if (ptr->inputs[i].isConnected()) { return i; }
        }
        return -1;
    }
};