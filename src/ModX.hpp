// TODO: Substep duration knob
// TODO: Substep shape knob :)
#include "biexpander/biexpander.hpp"
#include "constants.hpp"

using namespace constants;  // NOLINT
struct ModX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAM_REP_DUR, PARAM_GLIDE_TIME, PARAM_GLIDE_SHAPE, PARAMS_LEN };
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
    // TODO Factor out ModParams. Use direct getters instead if possible
    struct ModParams {
        bool glide = false;
        float glideTime = 0.0F;
        float glideShape = 0.0F;
        int reps = 1;
        float prob = 1.0F;
    };
    bool inPlace(int /*length*/, int /*channel*/) const override
    {
        return true;
    }

    ModParams getParams(int index) const
    {
        if (!ptr) { return ModParams{}; }
        return ModParams{getGlide(index), getGlideTime(), getGlideShape(), getReps(index),
                         getProb(index)};
    }
    bool getGlide(int index) const
    {
        if (!ptr || !ptr->inputs[ModX::INPUT_GLIDE].isConnected()) { return false; }
        return ptr->inputs[ModX::INPUT_GLIDE].getPolyVoltage(index) > BOOL_TRESHOLD;
    }
    float getGlideTime() const
    {
        return ptr->params[ModX::PARAM_GLIDE_TIME].getValue();
    }
    float getGlideShape() const
    {
        return ptr->params[ModX::PARAM_GLIDE_SHAPE].getValue();
    }
    int getReps(int index) const
    {
        if (!ptr || !ptr->inputs[ModX::INPUT_REPS].isConnected()) { return 1; }
        return ptr->inputs[ModX::INPUT_REPS].getPolyVoltage(index) * 16.0F / 10.0F;
    }
    float getProb(int index) const
    {
        if (!ptr || !ptr->inputs[ModX::INPUT_PROB].isConnected()) { return 1.F; }
        return ptr->inputs[ModX::INPUT_PROB].getPolyVoltage(index) / 10.0F;
    }
};
