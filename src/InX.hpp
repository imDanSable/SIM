#pragma once
#include <cassert>
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using namespace dimensions;  // NOLINT
struct InX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { ENUMS(INPUT_SIGNAL, 16), INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    InX();
    inline bool getInsertMode() const
    {
        return insertMode;
    }
    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;

   private:
    friend struct InXWidget;
    bool insertMode = false;
};

class InxAdapter : public biexpand::BaseAdapter<InX> {
   public:
    template <typename InIter, typename OutIter>
    OutIter transform(InIter first, InIter last, OutIter out, int channel = 0)
    {
        assert(ptr);
        int i = 0;
        bool connected = false;

        if (ptr->getInsertMode()) {
            int lastConnectedInputIndex = getLastConnectedInputIndex();

            for (int i = 0; i < lastConnectedInputIndex + 1; ++i, ++out) {
                // *out = ptr->inputs[i].getVoltage(channel);
                // else {
                //     *out = *first;
                //     ++first;
                // }
            }
            return out;

            if (lastConnectedInputIndex == -1) { return std::copy(first, last, out); }
            auto it = first;
            for (int i = 0; i < lastConnectedInputIndex + 1; ++i) {
                if (ptr->inputs[i].isConnected()) { *out = ptr->inputs[i].getVoltage(channel); }
                else {
                    *out = *it;
                    ++it;
                }
                ++out;
            }
            // std::copy(it, last, out);
            return out;
        }

        for (auto it = first; it != last && i < 16; ++it, ++out, ++i) {
            connected = ptr->inputs[i].isConnected();
            *out = connected ? ptr->inputs[i].getVoltage(channel) : *it;
            if (ptr->getInsertMode() && connected) { --it; }
        }
        return out;
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
    float getPolyVoltage(int port) const
    {
        if (!ptr) { return 0; }
        return ptr->inputs[port].getPolyVoltage(port);
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