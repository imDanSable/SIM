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
    template <typename Iter>
    ///@ Transform (in place)
    Iter ntransform(Iter first, Iter last, int channel = 0)
    {
        std::rotate(first, first + getStart(channel), last);
        std::advance(first, getLength(channel));
        return (first < last) ? first : last;
    }
    // template <typename InIter, typename OutIter>
    ///@ Transform (by copying)
    using BufIter = iters::BufIter;
    BufIter transform(BufIter first, BufIter last, BufIter out, int channel = 0) override
    {
        auto start = getStart(channel);
        auto length = getLength(channel);
        auto rem_length = length;

        auto mid = std::next(first + start, std::min(static_cast<std::ptrdiff_t>(length),
                                                     std::distance(first + start, last)));
        out = std::copy(first + start, mid, out);
        rem_length -= std::distance(first + start, mid);

        if (rem_length > 0) { out = std::copy(first, std::next(first, rem_length), out); }
        return out;
    }

    bool cvStartConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_START].isConnected();
    }

    bool cvLengthConnected()
    {
        return ptr && ptr->inputs[ReX::INPUT_LENGTH].isConnected();
    }

    int getLength(int channel = 0, int max = constants::NUM_CHANNELS) const
    {
        if (!ptr) { return constants::NUM_CHANNELS; }
        if (!ptr->inputs[ReX::INPUT_LENGTH].isConnected()) {
            return clamp(static_cast<int>(ptr->params[ReX::PARAM_LENGTH].getValue()), 1, max);
        }

        return clamp(static_cast<int>(rescale(ptr->inputs[ReX::INPUT_LENGTH].getVoltage(channel), 0,
                                              10, 1, static_cast<float>(max + 1))),
                     1, max);
    };
    int getStart(int channel = 0, int max = constants::NUM_CHANNELS) const
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
