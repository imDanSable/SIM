#include <cmath>

#include <array>
#include <deque>
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

using constants::NUM_CHANNELS;
class RingBuffer {
   public:
    RingBuffer() : maxSize(8) {}  // NOLINT

    void push(int value)
    {
        if (buffer.size() == maxSize) { buffer.pop_front(); }
        buffer.push_back(value);
        counter++;
    }

    int pop()
    {
        if (counter < maxSize) { return buffer.front(); }
        float value = buffer.front();
        counter--;
        return value;
    }

    void resize(size_t newSize)
    {
        maxSize = newSize;
        while (buffer.size() > maxSize) {
            buffer.pop_front();
        }
        counter = buffer.size();
    }

    size_t size() const
    {
        return buffer.size();
    }

   private:
    std::deque<float> buffer;
    size_t maxSize{};
    size_t counter{};
};
struct LegatoParams {
    explicit LegatoParams(float slewTime = 0, float from = 0, float to = 0)
        : slewTime(slewTime), from(from), to(to)
    {
    }
    void trigger(float slewTime, float from, float to, float shape)
    {
        active = true;
        this->slewTime = slewTime;
        this->from = from;
        this->to = to;
        this->progress = 0.0F;
        this->shape = clamp(shape, -0.99F, 0.99F);
        this->exponent = shape > 0.F ? 1 - shape : -(1 + shape);
    }
    float process(float sampleTime)
    {
        if (!active) { return NAN; }
        progress += sampleTime / slewTime;
        float shapedProgress = NAN;
        if ((shape > -0.04F) && (shape < 0.04F)) { shapedProgress = progress; }
        else if (shape < 0.F) {
            shapedProgress = 1.F - std::pow(1.F - progress, -exponent);
        }
        else if (shape > 0.F) {
            shapedProgress = std::pow(progress, exponent);
        }
        if ((shapedProgress >= 1.0F) || shapedProgress <= -1.0F) { return NAN; }
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
    float slewTime{};
    float from{};
    float to{};
    float progress{};
    float shape{};
    float exponent{};
};

struct Legato : Module {
    enum ParamId { SLEWTIME_PARAM, BIAS_PARAM, SHAPE_PARAM, SAMPLEDELAY_PARAM, PARAMS_LEN };
    enum InputId { SLEWTIME_INPUT, VOCT_INPUT, GATE_INPUT, INPUTS_LEN };
    enum OutputId { VOCT_OUTPUT, ACTIVE_OUTPUT, GATE_OUTPUT, OUTPUTS_LEN };
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

    RingBuffer ringBuf;  // Rinbuffer for floats with a max size of 5

   public:
    Legato()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam<ShapeQuantity>(SHAPE_PARAM, -1.F, 1.F, 0.F, "Response curve", "%", 0.F, 100.F);
        configParam(SAMPLEDELAY_PARAM, 0.F, 8.F, 1.F, "Sample delay for gate out", " samples")
            ->snapEnabled = true;
        configParam(SLEWTIME_PARAM, 0.F, .3F, .2F, "Slew rate", " seconds");
        configInput(VOCT_INPUT, "V/Oct");
        configInput(GATE_INPUT, "Gate");
        configInput(SLEWTIME_INPUT, "Slew Rate");
        configOutput(VOCT_OUTPUT, "V/Oct");
        configOutput(ACTIVE_OUTPUT, "Active: High during glide");
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
        int channels =
            std::max({1, inputs[SLEWTIME_INPUT].getChannels(), inputs[VOCT_INPUT].getChannels(),
                      inputs[GATE_INPUT].getChannels()});

        std::array<float, NUM_CHANNELS> cvIn{};
        std::array<float, NUM_CHANNELS> slewTime{};
        std::array<float, NUM_CHANNELS> gateIn{};
        std::array<float, NUM_CHANNELS> cvOut{};

        float shape = params[SHAPE_PARAM].getValue();
        bool cvInConnected = inputs[VOCT_INPUT].isConnected();
        bool gateConnected = inputs[GATE_INPUT].isConnected();
        bool slewTimeConnected = inputs[SLEWTIME_INPUT].isConnected();
        bool cvOutConnected = outputs[VOCT_OUTPUT].isConnected();
        bool newGate = false;
        bool noGate = true;
        if (!cvInConnected || !cvOutConnected) {
            outputs[VOCT_OUTPUT].setChannels(0);
            outputs[ACTIVE_OUTPUT].setChannels(0);
            outputs[GATE_OUTPUT].setChannels(0);
            return;
        }

        outputs[VOCT_OUTPUT].setChannels(channels);
        outputs[ACTIVE_OUTPUT].setChannels(channels);
        if (!gateConnected) {
            for (int channel = 0; channel < channels; channel++) {
                outputs[VOCT_OUTPUT].setVoltage(inputs[VOCT_INPUT].getPolyVoltage(channel),
                                                channel);
                outputs[ACTIVE_OUTPUT].setVoltage(0.F, channel);
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
                legatos[channel].trigger(slewTime[channel], lastCvIn[channel], cvIn[channel],
                                         shape);
            }
            if (newGate || ((noGate) && (cvIn[channel] != lastCvIn[channel]))) {
                // No glide
                cvOut[channel] = cvIn[channel];
                legatos[channel].reset();
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
            outputs[ACTIVE_OUTPUT].setVoltage(legatos[channel].isActive() ? 10.F : 0.F, channel);

            // Check if ringBuf Size is equal to the sample delay param
            if (ringBuf.size() != params[SAMPLEDELAY_PARAM].getValue()) {
                ringBuf.resize(1 + params[SAMPLEDELAY_PARAM].getValue());
            }
            // Push the gate value to the ring buffer
            ringBuf.push(inputs[GATE_INPUT].getPolyVoltage(channel));
            // Set the gate output to the value at the front of the ring buffer
            outputs[GATE_OUTPUT].setChannels(channels);
            outputs[GATE_OUTPUT].setVoltage(ringBuf.pop(), channel);
        }
    }
};

using namespace dimensions;  // NOLINT
struct LegatoWidget : ModuleWidget {
    explicit LegatoWidget(Legato* module)
    {
        float y = 25.F;
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
                                                Legato::ACTIVE_OUTPUT));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                Legato::VOCT_OUTPUT));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                Legato::GATE_OUTPUT));

        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, y += JACKYSPACE)), module,
                                                   Legato::SAMPLEDELAY_PARAM));
    }
};

Model* modelLegato = createModel<Legato, LegatoWidget>("Legato");  // NOLINT
