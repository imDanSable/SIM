#include "plugin.hpp"

namespace glide {

struct ShapeQuantity : ParamQuantity {
    std::string getUnit() override
    {
        float val = this->getValue();
        return val > 0.f ? "% log" : val < 0.f ? "% exp" : " = linear";
    }
};

struct GlideParams {
    explicit GlideParams(float glideTime = 1.F, float from = 0, float to = 0)
        : glideTime(glideTime), from(from), to(to)
    {
    }
    void trigger(float glideTime, float from, float to, float shape)
    {
        active = true;
        this->glideTime = glideTime;
        this->from = from;
        this->to = to;
        this->progress = 0.0F;
        this->shape = clamp(shape, -0.99F, 0.99F);
        this->exponent = shape > 0.F ? 1 - shape : -(1 + shape);
    }
    float processPhase(float phase)
    {
        if (glideTime == 0) { return this->to; }
        progress = phase / glideTime;
        return process(0.F);
    }
    float process(float sampleTime)
    {
        if (!active || glideTime == 0) { return this->to; }
        progress += sampleTime / glideTime;
        float shapedProgress = NAN;
        if ((shape > -0.005F) && (shape < 0.005F)) { shapedProgress = progress; }
        else if (shape < 0.F) {
            shapedProgress = clamp(1.F - std::pow(1.F - progress, -exponent), 0.F, 1.F);
        }
        else if (shape > 0.F) {
            shapedProgress = clamp(std::pow(progress, exponent), 0.F, 1.F);
        }
        if ((shapedProgress >= 1.0F) || shapedProgress <= -1.0F) { return this->to; }
        return from + (to - from) * shapedProgress;
    }
    void reset()
    {
        progress = 0.0F;
        active = false;
    }

    bool isActive() const
    {
        return active;
    }

   private:
    bool active{};
    float glideTime{};
    float from{};
    float to{};
    float progress{};
    float shape{};
    float exponent{};
};
}  // namespace glide