
#include "biexpander/biexpander.hpp"
#include "constants.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using namespace constants;  // NOLINT
struct ModX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_PROB, INPUT_REPS, INPUT_GLIDE, INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    ModX();

   private:
    friend struct ModXWidget;
};

class ModXAdapter : public biexpand::BaseAdapter<ModX> {
   private:
   public:
    struct ModParams {
        bool glide = false;
        float glideTime = 0.0F;
        float glideShape = 0.0F;
        int reps = 1;
        float prob = 1.0F;
        explicit operator bool() const
        {
            return !glide && reps == 1 && prob == 1.0F;
        }
    };
    iters::BoolIter transform(iters::BoolIter first,
                              iters::BoolIter last,
                              iters::BoolIter out,
                              int channel) override
    {
        /// XXX FIX THIS to in place
        return std::copy(first, last, out);
    }

    iters::FloatIter transform(iters::FloatIter first,
                               iters::FloatIter last,
                               iters::FloatIter out,
                               int channel) override
    {
        /// XXX FIX THIS to in place
        return std::copy(first, last, out);
    }
    ModParams getParams(int index) const
    {
        if (!ptr) { return ModParams{}; }
        return ModParams{getGlide(index), getGlideTime(index), getGlideShape(index), getReps(index),
                         getProb(index)};
    }
    bool getGlide(int index) const
    {
        if (!ptr) { return false; }
        return ptr->inputs[ModX::INPUT_GLIDE].getPolyVoltage(index) > BOOL_TRESHOLD;
    }
    float getGlideTime(int index) const
    {
        return 0.F;  // XXX Finish
    }
    float getGlideShape(int index) const
    {
        return 0.F;  // XXX Finish
    }
    int getReps(int index) const
    {
        if (!ptr) { return 1; }
        return ptr->inputs[ModX::INPUT_REPS].getPolyVoltage(index) * 16.0F / 10.0F;
    }
    float getProb(int index) const
    {
        if (!ptr) { return 1.F; }
        return ptr->inputs[ModX::INPUT_PROB].getPolyVoltage(index) / 10.0F;
    }
};
