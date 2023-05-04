#include "Expandable.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

// XXX connectEnds menu option
// XXX step output voltage mode menu option
using constants::LEFT;
using constants::RIGHT;

class Phi : public Expandable {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUT_CV, INPUT_PHASE, INPUTS_LEN };
    enum OutputId { NOTE_PHASE_OUTPUT, OUTPUT_NOTE_INDEX, TRIG_OUTPUT, OUTPUT_CV, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    friend struct PhiWidget;

    RexAdapter rex;
    OutxAdapter outx;
    InxAdapter inx;

    //@brief: Adjusts phase per channel so 10V doesn't revert back to zero.
    bool connectEnds = false;

    //@brief: The channel index last time we process()ed it.
    std::array<int, 16> prevChannelIndex = {0};

    //@brief: Pulse generators for gate out
    std::array<dsp::PulseGenerator, constants::NUM_CHANNELS> trigOutPulses;

    enum StepOutputVoltageMode {
        SCALE_10_TO_16,
        SCALE_1_TO_10,  // DOCB_COMPATIBLE
        SCALE_10_TO_LENGTH
    };

    StepOutputVoltageMode stepOutputVoltageMode = StepOutputVoltageMode::SCALE_10_TO_16;

   public:
    Phi()
        : Expandable({modelReX, modelInX}, {modelOutX}, LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(INPUT_CV, "Poly");
        configInput(INPUT_PHASE, "Phase clock");
        configOutput(NOTE_PHASE_OUTPUT, "Phase");
        configOutput(OUTPUT_NOTE_INDEX, "Current step");
        configOutput(TRIG_OUTPUT, "Step trigger");
        configOutput(OUTPUT_CV, "Main");
    }
    void updateRightExpanders() override
    {
        outx.setPtr(getExpanderAndSetCallbacks<OutX, RIGHT>({modelOutX}));
    }
    void updateLeftExpanders() override
    {
        inx.setPtr(getExpanderAndSetCallbacks<InX, LEFT>({modelInX, modelReX}));
        rex.setPtr(getExpanderAndSetCallbacks<ReX, LEFT>({modelReX}));
    }

    void process(const ProcessArgs& args) override
    {
        // Process CV IN
        const int inChannelCount = inputs[INPUT_CV].getChannels();
        const int phaseChannelCount = inputs[INPUT_PHASE].getChannels();
        outputs[OUTPUT_CV].setChannels(phaseChannelCount);
        for (int currPhaseChannel = 0; currPhaseChannel < phaseChannelCount; ++currPhaseChannel) {
            processChannel(args, currPhaseChannel, inChannelCount);
        }
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "connectEnds", json_boolean(connectEnds));
        json_object_set_new(rootJ, "stepOutputVoltageMode ",
                            json_integer(static_cast<int>(stepOutputVoltageMode)));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* connectEndsJ = json_object_get(rootJ, "connectEnds");
        if (connectEndsJ) { connectEnds = json_is_true(connectEndsJ); }
        json_t* stepOutputVoltageModeJ = json_object_get(rootJ, "stepOutputVoltageMode");
        if (stepOutputVoltageModeJ) {
            stepOutputVoltageMode =
                static_cast<StepOutputVoltageMode>(json_integer_value(stepOutputVoltageModeJ));
        }
    }

   private:
    void processChannel(const ProcessArgs& /*args*/, int channel, int inChannelCount)
    {
        if (inChannelCount == 0) {
            outputs[OUTPUT_CV].setVoltage(0.F, channel);
            return;
        }
        const float raw_phase = inputs[INPUT_PHASE].getNormalPolyVoltage(0, channel);
        const float phase = connectEnds ? clamp(raw_phase / 10.F, 0.F, 1.F)
                                        : clamp(raw_phase / 10.F, 0.00001F,
                                                .99999f);  // to avoid jumps at 0 and 1

        const int start = rex.getStart(channel);
        const int length = rex.getLength(channel);
        const int channel_index = (start + (static_cast<int>(phase * (length)))) % inChannelCount;
        if (prevChannelIndex[channel] != channel_index) {
            // XXX triggering
            prevChannelIndex[channel] = channel_index;
        }
        const float out_cv = inputs[INPUT_CV].getNormalPolyVoltage(0, channel_index);
        const float inx_cv = inx.getNormalPolyVoltage(out_cv, channel, channel_index);
        outputs[OUTPUT_CV].setVoltage(inx_cv, channel);
        outx.setVoltage(inx_cv, channel);
    };
};

using namespace dimensions;  // NOLINT

struct PhiWidget : ModuleWidget {
    explicit PhiWidget(Phi* module)
    {
        constexpr float centre = 1.5 * HP;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Phi.svg")));

        addInput(createInputCentered<SIMPort>(mm2px(Vec(centre, 16)), module, Phi::INPUT_CV));
        addInput(
            createInputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART)), module, Phi::INPUT_PHASE));

        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (4 * JACKNTXT))),
                                                module, Phi::NOTE_PHASE_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (5 * JACKNTXT))),
                                                module, Phi::OUTPUT_NOTE_INDEX));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (6 * JACKNTXT))),
                                                module, Phi::TRIG_OUTPUT));
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec(centre, JACKYSTART + (7 * JACKNTXT))),
                                                module, Phi::OUTPUT_CV));

        if (!module) { return; }
        module->addConnectionLights(this);
    };
};

Model* modelPhi = createModel<Phi, PhiWidget>("Phi");  // NOLINT
