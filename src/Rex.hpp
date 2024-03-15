#pragma once
#include <cstddef>
#include <variant>
#include "InX.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/Module.hpp"
#include "plugin.hpp"

struct ReX : public biexpand::LeftExpander {
    enum ParamId { PARAM_START, PARAM_LENGTH, PARAMS_LEN };
    enum InputId { INPUT_START, INPUT_LENGTH, INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    ReX();
};

class RexAdapter : public biexpand::BaseAdapter<ReX> {
   public:
    using FloatIter = iters::FloatIter;
    using BoolIter = iters::BoolIter;

   private:
    template <typename Iter>
    Iter transformImpl(Iter first, Iter last, Iter out, int channel = 0)
    {
        ///XXX It seems that channel > 0 gets a small input while channel 0 gets the full input
        const auto start = getStart(channel);
        const auto length = getLength(channel);
        const auto outputStart = out;
        const auto inputSize = last - first;
        int debugLen = 0;

        // Debug the input first:
        // if (true) {  // (channel == 1) {
        //     DEBUG("Channel: %d", channel);
        //     DEBUG("InStart: %d, InLength: %d", start, length);
        //     std::string s;
        //     for (auto it = first; it != last; ++it) {
        //         s += static_cast<bool>(*it) ? "1" : "0";
        //     }
        //     s += "\n";
        //     DEBUG("Input %s", s.c_str());
        // }

        Iter retVal = {};
        if (first + start + length <= last) {
            // If no wrap-around is needed, just perform a single copy operation
            retVal = std::copy(first + start, first + start + length, out);
            debugLen = retVal - outputStart;
            return retVal;
        }
        // If wrap-around is needed

        // Do we start before the end of the input?
        if (first + start < last) { out = std::copy(first + start, last, out); }

        retVal = out;
        debugLen = retVal - outputStart;

        // And repeat copying full inputs
        while (out - outputStart + inputSize < length) {
            out = std::copy(first, last, out);
            retVal = out;
        }

        debugLen = retVal - outputStart;

        // And finally, copy the remainder
        retVal = std::copy_n(first, length - (out - outputStart), out);

        debugLen = retVal - outputStart;

        // if (true) {  // (channel == 1) {
        //     DEBUG("Channel: %d", channel);
        //     DEBUG("OutLength: %td", retVal - outputStart);
        //     std::string s;
        //     for (auto it = outputStart; it != retVal; ++it) {
        //         s += static_cast<bool>(*it) ? "1" : "0";
        //     }
        //     DEBUG("Output %s", s.c_str());
        // }
        return retVal;
    }

   public:
    // template <typename BufIter>
    ///@ Transform (in place)
    FloatIter ntransform(FloatIter first, FloatIter last, int channel = 0) const
    {
        std::rotate(first, first + getStart(channel), last);
        std::advance(first, getLength(channel));
        return (first < last) ? first : last;
    };
    BoolIter transform(BoolIter first, BoolIter last, BoolIter out, int channel) override
    {
        // copy all for debugging
        // return out;
        // std::copy(first, last, out);
        return transformImpl(first, last, out, channel);
    }
    // ///@ Transform (by copying)
    FloatIter transform(FloatIter first, FloatIter last, FloatIter out, int channel) override
    {
        return transformImpl(first, last, out, channel);
    }

    bool cvStartConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_START].isConnected();
    }

    bool cvLengthConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_LENGTH].isConnected();
    }

    int getLength(int channel = 0, int max = constants::MAX_GATES) const
    {
        if (!ptr) { return constants::MAX_GATES; }
        if (!ptr->inputs[ReX::INPUT_LENGTH].isConnected()) {
            return clamp(static_cast<int>(ptr->params[ReX::PARAM_LENGTH].getValue()), 1, max);
        }

        return clamp(static_cast<int>(rescale(ptr->inputs[ReX::INPUT_LENGTH].getVoltage(channel), 0,
                                              10, 1, static_cast<float>(max + 1))),
                     1, max);
    };
    int getStart(int channel = 0, int max = constants::MAX_GATES) const
    {
        if (!ptr) { return 0; }
        if (!ptr->inputs[ReX::INPUT_START].isConnected()) {
            return clamp(static_cast<int>(ptr->params[ReX::PARAM_START].getValue()), 0, max - 1);
        }
        return clamp(static_cast<int>(rescale(ptr->inputs[ReX::INPUT_START].getPolyVoltage(channel),
                                              0, 10, 0, static_cast<float>(max))),
                     0, max - 1);
    };
};
