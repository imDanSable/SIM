#pragma once
#include <rack.hpp>
#include "biexpander/biexpander.hpp"
#include "constants.hpp"
#include "helpers/iters.hpp"

using namespace rack;  // NOLINT
// using iters::operator!=;  // NOLINT

struct ReX : public biexpand::BiExpander {
    enum ParamId { PARAM_START, PARAM_LENGTH, PARAMS_LEN };
    enum InputId { INPUT_START, INPUT_LENGTH, INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };
    ReX();
};

class RexAdapter : public biexpand::BaseAdapter<ReX> {
   public:
    using FloatIter = iters::FloatIter;
    using BoolIter = iters::BoolIter;

   private:
    template <typename Iter>
    Iter transformImpl(Iter first, Iter last, Iter out, int channel = 0) const
    {
        if (first == last) { return out; }
        const auto start = getStart(channel);
        const auto length = getLength(channel);
        const auto outputStart = out;
        const auto inputSize = last - first;

        if (first + start + length <= last) {
            // If no wrap-around is needed, just perform a single copy operation
            return std::copy(first + start, first + start + length, out);
        }

        // Do we start before the end of the input?
        if (first + start < last) { out = std::copy(first + start, last, out); }

        // And repeat copying full inputs
        while (out - outputStart + inputSize < length) {
            out = std::copy(first, last, out);
        }
        // And finally, copy the remainder
        return std::copy_n(first, length - (out - outputStart), out);
    }

    mutable std::vector<Param> prevParams;
    mutable std::vector<Input> prevInputs;

   public:
    bool inPlace(int length, int channel) const override
    {
        return getLength(channel) == length;
    }
    ///@ Transform (in place)
    void transformInPlace(FloatIter first, FloatIter last, int channel) const override
    {
        const auto totalElements = std::distance(first, last);
        const auto startOffset = getStart(channel) % totalElements;

        auto newStartIterator = first;
        std::advance(newStartIterator, startOffset);

        std::rotate(first, newStartIterator, last);
    };

    ///@ Transform (in place)
    void transformInPlace(BoolIter first, BoolIter last, int channel) const override  // XXX DOUBLE
    {
        const auto totalElements = std::distance(first, last);
        const auto startOffset = getStart(channel) % totalElements;

        auto newStartIterator = first;
        std::advance(newStartIterator, startOffset);

        std::rotate(first, newStartIterator, last);
    };
    BoolIter transform(BoolIter first, BoolIter last, BoolIter out, int channel) const override
    {
        return transformImpl(first, last, out, channel);
    }
    // ///@ Transform (by copying)
    FloatIter transform(FloatIter first, FloatIter last, FloatIter out, int channel) const override
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
