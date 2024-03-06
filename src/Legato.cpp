// TODO: Implement curve adjustment

#include <array>
#include <cmath>
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

/* Code from bogaudio to learn from:

float RiseFallShapedSlewLimiter::timeMS(int c, Param& param, Input* input, float maxMS) {
        float time = clamp(param.getValue(), 0.0f, 1.0f);
        if (input && input->isConnected()) {
                time *= clamp(input->getPolyVoltage(c) / 10.0f, 0.0f, 1.0f);
        }
        return time * time * maxMS;
}
void ShapedSlewLimiter::setParams(float sampleRate, float milliseconds, float shape) {
        assert(sampleRate > 0.0f);
        assert(milliseconds >= 0.0f);
        assert(shape >= minShape);
        assert(shape <= maxShape);
        _sampleTime = 1.0f / sampleRate;
        _time = milliseconds / 1000.0f;
        _shapeExponent = (shape > -0.05f && shape < 0.05f) ? 0.0f : shape;
        _inverseShapeExponent = 1.0f / _shapeExponent;
}

float ShapedSlewLimiter::next(float sample) {
        double difference = sample - _last;
        double ttg = fabs(difference) / range;
        if (_time < 0.0001f) {
                return _last = sample;
        }
        if (_shapeExponent != 0.0f) {
                ttg = pow(ttg, _shapeExponent);
        }
        ttg *= _time;
        ttg = std::max(0.0, ttg - _sampleTime);
        ttg /= _time;
        if (_shapeExponent != 0.0f) {
                ttg = pow(ttg, _inverseShapeExponent);
        }
        double y = fabs(difference) - ttg * range;
        if (difference < 0.0f) {
                return _last = std::max(_last - y, (double)sample);
        }
        return _last = std::min(_last + y, (double)sample);
}

float RiseFallShapedSlewLimiter::shape(int c, Param& param, bool invert, Input* cv, ShapeCVMode
mode) { float shape = param.getValue(); if (invert) { shape *= -1.0f;
        }
        if (cv && mode != OFF_SCVM) {
                float v = cv->getPolyVoltage(c) / 5.0f;
                if (mode == INVERTED_SCVM) {
                        v = -v;
                }
                shape = clamp(shape + v, -1.0f, 1.0f);
        }
        if (shape < 0.0) {
                shape = 1.0f + shape;
                shape = _rise.minShape + shape * (1.0f - _rise.minShape);
        }
        else {
                shape += 1.0f;
        }
        return shape;
}

void RiseFallShapedSlewLimiter::modulate(
        float sampleRate,
        Param& riseParam,
        Input* riseInput,
        float riseMaxMS,
        Param& riseShapeParam,
        Param& fallParam,
        Input* fallInput,
        float fallMaxMS,
        Param& fallShapeParam,
        int c,
        bool invertRiseShape,
        Input* shapeCV,
        ShapeCVMode riseShapeMode,
        ShapeCVMode fallShapeMode
) {
        _rise.setParams(
                sampleRate,
                timeMS(c, riseParam, riseInput, riseMaxMS),
                shape(c, riseShapeParam, invertRiseShape, shapeCV, riseShapeMode)
        );
        _fall.setParams(
                sampleRate,
                timeMS(c, fallParam, fallInput, fallMaxMS),
                shape(c, fallShapeParam, false, shapeCV, fallShapeMode)
        );
}

float RiseFallShapedSlewLimiter::next(float sample) {
        if (sample > _last) {
                if (!_rising) {
                        _rising = true;
                        _rise._last = _last;
                }
                return _last = _rise.next(sample);
        }

        if (_rising) {
                _rising = false;
                _fall._last = _last;
        }
        return _last = _fall.next(sample);
}
*/
using constants::NUM_CHANNELS;
struct LegatoParams {
    explicit LegatoParams(float slewTime = 0, float from = 0, float to = 0)
        : slewTime(slewTime), from(from), to(to)
    {
    }
    void trigger(float slewTime, float from, float to)
    {
        active = true;
        this->slewTime = slewTime;
        this->from = from;
        this->to = to;
        this->progress = 0.0F;
    }
    float process(float sampleTime)
    {
        if (!active) { return NAN; }
        progress += sampleTime / slewTime;
        if ((progress >= 1.0F) || progress <= -1.0F) { return NAN; }
        return from + (to - from) * progress;
    }
    void reset()
    {
        progress = 0.0F;
        active = false;
    }

   private:
    bool active{};
    float slewTime{};
    float from{};
    float to{};
    float progress{};
};

struct Legato : Module {
    enum ParamId { SLEWTIME_PARAM, BIAS_PARAM, SHAPE_PARAM, PARAMS_LEN };
    enum InputId { SLEWTIME_INPUT, VOCT_INPUT, GATE_INPUT, INPUTS_LEN };
    enum OutputId { VOCT_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };
    struct ShapeQuantity : ParamQuantity {
        std::string getUnit() override
        {
            float val = this->getValue();
            return val > 0.f ? "% log" : val < 0.f ? "% exp" : " = linear";
        }
    };

   private:
    std::array<LegatoParams, NUM_CHANNELS> legatos;
    std::array<float, NUM_CHANNELS> lastCvIn{};
    std::array<float, NUM_CHANNELS> lastGate{};

   public:
    Legato()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam<ShapeQuantity>(SHAPE_PARAM, -1.F, 1.F, 0.F, "Response curve", "%", 0.F, 100.F);
        configParam(SLEWTIME_PARAM, 0.F, 1.F, .1F, "Slew rate", "s");
        configInput(VOCT_INPUT, "V/Oct");
        configInput(GATE_INPUT, "Gate");
        configInput(SLEWTIME_INPUT, "Slew Rate");
        configOutput(VOCT_OUTPUT, "V/Oct");
    }

    void processBypass(const ProcessArgs& /*args*/) override
    {
        int channels =
            std::max({1, inputs[SLEWTIME_INPUT].getChannels(), inputs[VOCT_INPUT].getChannels()});
        for (int channel = 0; channel < channels; channel++) {
            outputs[VOCT_OUTPUT].setVoltage(inputs[VOCT_INPUT].getPolyVoltage(channel), channel);
        }
    }
    void process(const ProcessArgs& args) override
    {
        // get channels
        int channels =
            std::max({1, inputs[SLEWTIME_INPUT].getChannels(), inputs[VOCT_INPUT].getChannels()});

        std::array<float, NUM_CHANNELS> cvIn{};
        std::array<float, NUM_CHANNELS> slewTime{};
        std::array<float, NUM_CHANNELS> gateIn{};
        std::array<float, NUM_CHANNELS> cvOut{};

        bool cvInConnected = inputs[VOCT_INPUT].isConnected();
        bool gateConnected = inputs[GATE_INPUT].isConnected();
        bool slewTimeConnected = inputs[SLEWTIME_INPUT].isConnected();
        bool cvOutConnected = outputs[VOCT_OUTPUT].isConnected();
        bool newGate = false;
        bool noGate = true;
        if (!cvInConnected || !cvOutConnected) {
            outputs[VOCT_OUTPUT].setChannels(0);
            return;
        }

        outputs[VOCT_OUTPUT].setChannels(channels);
        if (!gateConnected) {
            for (int s = 0; s < channels; s++) {
                outputs[VOCT_OUTPUT].setVoltage(inputs[VOCT_INPUT].getPolyVoltage(s));
            }
            return;
        }

        for (int channel = 0; channel < channels; channel++) {
            gateIn[channel] = inputs[GATE_INPUT].getPolyVoltage(channel);
            newGate = gateIn[channel] > lastGate[channel];
            noGate = gateIn[channel] < 1.0F;
            cvIn[channel] = inputs[VOCT_INPUT].getPolyVoltage(channel);
            if (slewTimeConnected) {
                slewTime[channel] = clamp(inputs[SLEWTIME_INPUT].getPolyVoltage(channel) *
                                              params[SLEWTIME_PARAM].getValue() / 10.F,
                                          0.0F, 1.0F);
            }
            else {
                slewTime[channel] = params[SLEWTIME_PARAM].getValue();
            }

            if ((!noGate && !newGate) &&
                (cvIn[channel] != lastCvIn[channel])) {  // There's a gate and its not new and
                                                         // there's a change in the CV
                // Initiate the glide
                legatos[channel].trigger(slewTime[channel], lastCvIn[channel], cvIn[channel]);
            }
            if (newGate || ((noGate) && (cvIn[channel] != lastCvIn[channel]))) {
                // No glide
                cvOut[channel] = cvIn[channel];
            }
            else {
                float newValue = legatos[channel].process(args.sampleTime);
                if (std::isnan(newValue)) {
                    legatos[channel].reset();
                    cvOut[channel] = cvIn[channel];
                }
                else {
                    cvOut[channel] = newValue;
                }
            }
            lastCvIn[channel] = cvIn[channel];
            lastGate[channel] = gateIn[channel];
            outputs[VOCT_OUTPUT].setVoltage(cvOut[channel], channel);
        }
    }
};

using namespace dimensions;  // NOLINT
struct LegatoWidget : ModuleWidget {
    explicit LegatoWidget(Legato* module)
    {
        float y = 40.F;
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Legato.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Legato::VOCT_INPUT));
        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                              Legato::GATE_INPUT));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                              Legato::SLEWTIME_PARAM));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y += JACKYSPACE)), module,
                                              Legato::SLEWTIME_INPUT));

        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                   Legato::SHAPE_PARAM));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                Legato::VOCT_OUTPUT));
    }
};

Model* modelLegato = createModel<Legato, LegatoWidget>("Legato");
