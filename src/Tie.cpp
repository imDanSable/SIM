#include <array>
#include <cmath>
#include <deque>
#include "components.hpp"
#include "constants.hpp"
#include "glide.hpp"
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

using namespace glide;  // NOLINT

struct Tie : Module {
    enum ParamId { GLIDETIME_PARAM, BIAS_PARAM, SHAPE_PARAM, SAMPLEDELAY_PARAM, PARAMS_LEN };
    enum InputId { INPUT_GLIDETIME_CV, VOCT_INPUT, GATE_INPUT, INPUTS_LEN };
    enum OutputId { VOCT_OUTPUT, ACTIVE_OUTPUT, GATE_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

   private:
    std::array<GlideParams, NUM_CHANNELS> legatos;
    std::array<float, NUM_CHANNELS> lastCvIn{};
    std::array<float, NUM_CHANNELS> lastGate{};

    RingBuffer ringBuf;  // Ringbuffer for floats with a max size of 5

   public:
    Tie()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam<ShapeQuantity>(SHAPE_PARAM, -1.F, 1.F, 0.F, "Response curve", "%", 0.F, 100.F);
        configParam(SAMPLEDELAY_PARAM, 0.F, 8.F, 1.F, "Sample delay for gate out", " samples")
            ->snapEnabled = true;
        configParam(GLIDETIME_PARAM, 0.F, .3F, .1F, "Glide time", " seconds");
        configInput(VOCT_INPUT, "V/Oct");
        configInput(GATE_INPUT, "Gate");
        configInput(INPUT_GLIDETIME_CV, "Glide time CV");
        configOutput(VOCT_OUTPUT, "V/Oct");
        configOutput(ACTIVE_OUTPUT, "Active: High during glide");
    }

    void processBypass(const ProcessArgs& /*args*/) override
    {
        int channels = std::max(
            {1, inputs[INPUT_GLIDETIME_CV].getChannels(), inputs[VOCT_INPUT].getChannels()});
        for (int channel = 0; channel < channels; channel++) {
            outputs[VOCT_OUTPUT].setVoltage(inputs[VOCT_INPUT].getPolyVoltage(channel), channel);
        }
    }
    void process(const ProcessArgs& args) override
    {
        int channels =
            std::max({1, inputs[INPUT_GLIDETIME_CV].getChannels(), inputs[VOCT_INPUT].getChannels(),
                      inputs[GATE_INPUT].getChannels()});

        std::array<float, NUM_CHANNELS> cvIn{};
        std::array<float, NUM_CHANNELS> glideTime{};
        std::array<float, NUM_CHANNELS> gateIn{};
        std::array<float, NUM_CHANNELS> cvOut{};

        float shape = params[SHAPE_PARAM].getValue();
        bool cvInConnected = inputs[VOCT_INPUT].isConnected();
        bool gateConnected = inputs[GATE_INPUT].isConnected();
        bool glideTimeConnected = inputs[INPUT_GLIDETIME_CV].isConnected();
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
            if (glideTimeConnected) {
                glideTime[channel] = clamp(inputs[INPUT_GLIDETIME_CV].getPolyVoltage(channel) *
                                               params[GLIDETIME_PARAM].getValue() / 10.F,
                                           0.0F, 1.0F);
            }
            else {
                glideTime[channel] = params[GLIDETIME_PARAM].getValue();
            }

            if ((!noGate && !newGate) &&
                (cvIn[channel] != lastCvIn[channel])) {  // There's a gate and its not new and
                                                         // there's a change in the CV
                // Initiate the glide
                legatos[channel].trigger(glideTime[channel], lastCvIn[channel], cvIn[channel],
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
struct TieWidget : public SIMWidget {
    explicit TieWidget(Tie* module)
    {
        float y = 25.F;
        setModule(module);
        setSIMPanel("Tie");

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y)), module, Tie::VOCT_INPUT));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module, Tie::GATE_INPUT));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                              Tie::GLIDETIME_PARAM));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(HP, y += JACKYSPACE)), module,
                                              Tie::INPUT_GLIDETIME_CV));

        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                   Tie::SHAPE_PARAM));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module,
                                                Tie::ACTIVE_OUTPUT));

        addOutput(
            createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module, Tie::VOCT_OUTPUT));

        addOutput(
            createOutputCentered<SIMPort>(mm2px(Vec(HP, y += JACKNTXT)), module, Tie::GATE_OUTPUT));

        addParam(createParamCentered<SIMSmallKnob>(mm2px(Vec(HP, y += JACKYSPACE)), module,
                                                   Tie::SAMPLEDELAY_PARAM));
    }
};

Model* modelTie = createModel<Tie, TieWidget>("Tie");  // NOLINT
