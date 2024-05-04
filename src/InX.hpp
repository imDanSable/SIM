#pragma once
#include <cassert>
#include "biexpander/biexpander.hpp"
#include "constants.hpp"
#include "helpers/iters.hpp"

using namespace dimensions;  // NOLINT
struct InX : biexpand::BiExpander {
   public:
    enum ParamId { PARAM_INSERTMODE, PARAMS_LEN };
    enum InputId { ENUMS(INPUT_SIGNAL, 16), INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    InX();
    enum class InsertMode { OVERWRITE, INSERT, ADD_AND };
    InsertMode getInsertMode() const
    {
        return static_cast<InsertMode>(
            const_cast<InX*>(this)->params[PARAM_INSERTMODE].getValue());  // NOLINT
    }
    void setInsertMode(InsertMode mode)
    {
        params[PARAM_INSERTMODE].setValue(static_cast<float>(mode));
    }

   private:
    friend struct InXWidget;
};

// Import BufIter from iters so we can use it
class InxAdapter : public biexpand::BaseAdapter<InX> {
   private:
    constexpr static const float BOOLTRIGGER = 1.F;
    template <typename Iter>
    Iter transformImpl(Iter first, Iter last, Iter out, int channel = 0) const
    {
        assert(ptr);
        const InX::InsertMode mode = ptr->getInsertMode();
        int channel_counter = 0;

        int lastConnectedInputIndex = getLastConnectedInputIndex();
        if (lastConnectedInputIndex == -1) { return std::copy(first, last, out); }
        auto original = first;
        // Loop over inx ports
        for (int inx_port = 0; (inx_port < lastConnectedInputIndex + 1) &&
                               (channel_counter < constants::NUM_CHANNELS);
             ++inx_port) {
            if (ptr->inputs[inx_port].isConnected()) {
                // Loop over inx.port channels of the connected port
                for (int port_channel = 0; port_channel < ptr->inputs[inx_port].getChannels();
                     ++port_channel) {
                    // Bool?
                    if (std::is_same_v<Iter, iters::BoolIter>) {
                        // the mode is insert (otherwise it would be transformImplInPlace)
                        // if (mode == InX::InsertMode::INSERT):
                        *out = ptr->inputs[inx_port].getPolyVoltage(port_channel) > BOOLTRIGGER;
                    }
                    // Float?
                    else {
                        std::function<float(float)> f = this->getFloatValueFunction();
                        if (f) {
                            *out = f(ptr->inputs[inx_port].getPolyVoltage(port_channel) +
                                     ((mode == InX::InsertMode::ADD_AND) ? *original : 0.F));
                        }
                        else {
                            *out = ptr->inputs[inx_port].getPolyVoltage(port_channel) +
                                   ((mode == InX::InsertMode::ADD_AND) ? *original : 0.F);
                        }
                    }
                    ++channel_counter;
                    ++out;
                    if (channel_counter == constants::NUM_CHANNELS) {
                        return out;
                    }  // Maximum number of channels reached
                }
            }
            // if There's are still items in the input, copy one
            if (original != last) {
                if constexpr (std::is_same_v<Iter, iters::BoolIter>) { *out = *original; }
                else if constexpr (std::is_same_v<Iter, iters::FloatIter>) {
                    std::function<float(float)> f = this->getFloatValueFunction();
                    if (f) { *out = f(*original); }
                    else {
                        *out = *original;
                    }
                }
                // *out = *input;
                ++original;
                ++out;
                ++channel_counter;
            }
        }
        // Copy the rest of the input using std::copy keeping an eye on the channel counter
        while (original != last && channel_counter < constants::NUM_CHANNELS) {
            if constexpr (std::is_same_v<Iter, iters::BoolIter>) { *out = *original; }
            else if constexpr (std::is_same_v<Iter, iters::FloatIter>) {
                std::function<float(float)> f = this->getFloatValueFunction();
                if (f) { *out = f(*original); }
                else {
                    *out = *original;
                }
            }
            ++original;
            ++out;
            ++channel_counter;
        }
        return out;
    }

   public:
    bool inPlace(int /*length*/, int /*channel*/) const override
    {
        return !(getInsertMode() == InX::InsertMode::INSERT);
    }

    template <typename Iter>
    void transformImplInPlace(Iter first, Iter last, Iter out, int channel = 0) const
    {
        const InX::InsertMode mode = ptr->getInsertMode();
        int i = 0;
        for (auto it = first; it != last && i < 16; ++it, ++out, ++i) {
            bool connected = ptr->inputs[i].isConnected();
            // Bool?
            if constexpr (std::is_same_v<Iter, iters::BoolIter>) {
                switch (mode) {
                    case InX::InsertMode::ADD_AND:
                        *out = connected ? (ptr->inputs[i].getVoltage(channel) > BOOLTRIGGER) && *it
                                         : *it;
                        break;
                    default:
                        *out = connected ? ptr->inputs[i].getVoltage(channel) > BOOLTRIGGER : *it;
                        break;
                }
            }
            else if constexpr (std::is_same_v<Iter, iters::FloatIter>) {
                // Float with quantize?
                if (this->getFloatValueFunction()) {
                    *out = connected ? this->getFloatValueFunction()(
                                           ptr->inputs[i].getVoltage(channel) +
                                           (mode == InX::InsertMode::ADD_AND ? *it : 0.F))
                                     : *it;
                }
                // Float without quantize
                else {
                    *out = connected ? ptr->inputs[i].getVoltage(channel) +
                                           (mode == InX::InsertMode::ADD_AND ? *it : 0.F)
                                     : *it;
                }
                // *out = connected ? ptr->inputs[i].getVoltage(channel) : *it;
            }
            if ((mode == InX::InsertMode::INSERT) && connected) { --it; }
        }
    }
    void transformInPlace(iters::FloatIter first, iters::FloatIter last, int channel) const override
    {
        transformImplInPlace(first, last, first, channel);
    }
    void transformInPlace(iters::BoolIter first, iters::BoolIter last, int channel) const override
    {
        transformImplInPlace(first, last, first, channel);
    }

    iters::FloatIter transform(iters::FloatIter first,
                               iters::FloatIter last,
                               iters::FloatIter out,
                               int channel) const override
    {
        return transformImpl(first, last, out, channel);
    }
    iters::BoolIter transform(iters::BoolIter first,
                              iters::BoolIter last,
                              iters::BoolIter out,
                              int channel) const override
    {
        return transformImpl(first, last, out, channel);
    }

    InX::InsertMode getInsertMode() const
    {
        if (!ptr) { return InX::InsertMode::OVERWRITE; }
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
    float* getVoltages(int port) const
    {
        if (!ptr) { return nullptr; }
        return ptr->inputs[port].getVoltages();
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
        return ptr && ptr->inputs[port].isConnected();
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