#include <array>
#include <utility>
#include "components.hpp"
#include "plugin.hpp"

struct Coerce : Module {
    enum ParamId { PARAMS_LEN };
    enum InputId { SELECTIONS1_INPUT, IN1_INPUT, INPUTS_LEN };
    enum OutputId { OUT1_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };
    int INPUTS_AND_OUTPUTS = 1;

    enum class RestrictMethod {
        RESTRICT,     // Only allow values from the selection
        OCTAVE_FOLD,  // Allow values from the selection, but fold them into their own octave
    };

    enum class RoundingMethod { CLOSEST, DOWN, UP };

    RestrictMethod restrictMethod{};
    RoundingMethod roundingMethod{};

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "restrictMethod", json_integer((int)restrictMethod));
        json_object_set_new(rootJ, "roundingMethod", json_integer((int)roundingMethod));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* restrictMethodJ = json_object_get(rootJ, "restrictMethod");
        if (restrictMethodJ) restrictMethod = (RestrictMethod)json_integer_value(restrictMethodJ);

        json_t* roundingMethodJ = json_object_get(rootJ, "roundingMethod");
        if (roundingMethodJ) roundingMethod = (RoundingMethod)json_integer_value(roundingMethodJ);
    }

    bool getNormalledSelections(const int input_idx,
                                int& sel_count,
                                int& sel_id,
                                std::array<float, 16>& selections)
    {
        for (int i = input_idx; i >= 0; i--) {
            if (inputs[SELECTIONS1_INPUT + i].isConnected()) {
                sel_count = inputs[SELECTIONS1_INPUT + i].getChannels();
                sel_id = SELECTIONS1_INPUT + i;

                inputs[SELECTIONS1_INPUT + i].readVoltages(selections.data());
                return true;
            }
        }
        sel_count = 0;
        sel_id = 0;
        return false;
    }

    void onPortChange(const PortChangeEvent& e) override
    {
        // Reset output channels when input is disconnected
        if (!e.connecting && (e.type == Port::Type::INPUT)) {
            // Input disconnect
            if (e.portId >= IN1_INPUT && e.portId <= IN1_INPUT + INPUTS_AND_OUTPUTS) {
                int input_idx = e.portId - IN1_INPUT;
                if (outputs[OUT1_OUTPUT + input_idx].isConnected()) {
                    outputs[OUT1_OUTPUT + input_idx].setChannels(0);
                }
            }
        }
    };
    void adjustValues(int quantizeInputId,
                      float input[],
                      float output[],
                      int inputLen,
                      float quantize[],
                      int selectionsLen)
    {
        switch (restrictMethod) {
            case RestrictMethod::RESTRICT: {
                for (int i = 0; i < inputLen; i++) {
                    float value = input[i];
                    float closest = NAN;
                    float biggest = -1000000.0F;
                    float smallest = 1000000.0F;
                    float min_diff = 1000000.0F;

                    for (int j = 0; j < selectionsLen; j++) {
                        float diff = fabsf(value - quantize[j]);
                        if (roundingMethod == RoundingMethod::CLOSEST) {
                            if (diff < min_diff) {
                                closest = quantize[j];
                                min_diff = diff;
                            }
                        }
                        else {
                            if (roundingMethod == RoundingMethod::DOWN) {
                                if (quantize[j] < smallest) { smallest = quantize[j]; }
                                if ((quantize[j] <= value) && (diff < min_diff)) {
                                    closest = quantize[j];
                                    min_diff = diff;
                                }
                            }
                            else if (roundingMethod == RoundingMethod::UP) {
                                if (quantize[j] >= biggest) { biggest = quantize[j]; }
                                if ((quantize[j] >= value) && (diff < min_diff)) {
                                    closest = quantize[j];
                                    min_diff = diff;
                                }
                            }
                        }
                    }
                    if ((roundingMethod == RoundingMethod::DOWN) && (value < smallest)) {
                        closest = smallest;
                    }
                    else if ((roundingMethod == RoundingMethod::UP) && (value > biggest)) {
                        closest = biggest;
                    }
                    output[i] = closest;
                }
                break;
            }
            case RestrictMethod::OCTAVE_FOLD: {
                auto roundingCondition = [](float x, RoundingMethod roundMethod) -> bool {
                    switch (roundMethod) {
                        case RoundingMethod::CLOSEST: return true;
                        case RoundingMethod::DOWN: return (x >= 0);
                        case RoundingMethod::UP: return (x <= 0);
                    }
                    return false;
                };
                for (int i = 0; i < inputLen; i++) {
                    float min_diff = 1000000.0F;
                    int closestIdx = 0;
                    int addOctave = 0;
                    float inputFraction = (input[i] - floor(input[i]));
                    for (int j = 0; j < selectionsLen; j++) {
                        const float quantizeFraction = quantize[j] - floor(quantize[j]);
                        const float diff = inputFraction - quantizeFraction;
                        const float diff2 = inputFraction - (quantizeFraction - 1.0F);
                        const float diff3 = inputFraction - (quantizeFraction + 1.0F);

                        if ((abs(diff) < abs(min_diff)) || (abs(diff2) < abs(min_diff)) ||
                            (abs(diff3) < abs(min_diff))) {
                            if ((abs(diff) < abs(min_diff)) &&
                                roundingCondition(diff, roundingMethod)) {
                                closestIdx = j;
                                min_diff = diff;
                                addOctave = 0;
                            }
                            else if ((abs(diff2) < abs(min_diff)) &&
                                     roundingCondition(diff2, roundingMethod)) {
                                closestIdx = j;
                                min_diff = diff2;
                                addOctave = -1;
                            }
                            else if ((abs(diff3) < abs(min_diff)) &&
                                     roundingCondition(diff3, roundingMethod)) {
                                closestIdx = j;
                                min_diff = diff3;
                                addOctave = 1;
                            }
                        }
                    }
                    output[i] = floor(input[i]) + addOctave +
                                abs(quantize[closestIdx] - floor(quantize[closestIdx]));
                }
                break;
            }
            default: break;
        }
    }

    void onReset(const ResetEvent& e) override
    {
        restrictMethod = RestrictMethod::OCTAVE_FOLD;
        roundingMethod = RoundingMethod::CLOSEST;
    }

    void process(const ProcessArgs& args) override
    {
        for (int i = 0; i < INPUTS_AND_OUTPUTS; i++) {
            if (inputs[IN1_INPUT + i].isConnected() && outputs[OUT1_OUTPUT + i].isConnected()) {
                int sel_count = 0;
                int sel_id = 0;
                std::array<float, 16> selections = {};
                outputs[OUT1_OUTPUT + i].setChannels(inputs[IN1_INPUT + i].getChannels());
                if (getNormalledSelections(i, sel_count, sel_id, selections)) {
                    adjustValues(sel_id, inputs[IN1_INPUT + i].getVoltages(),
                                 outputs[OUT1_OUTPUT + i].getVoltages(),
                                 inputs[IN1_INPUT + i].getChannels(), selections.data(), sel_count);
                }
                else {
                    outputs[OUT1_OUTPUT + i].setVoltage(inputs[IN1_INPUT + i].getVoltage());
                }
            }
        }
    }
};
struct Coerce6 : Coerce {
    enum ParamId { PARAMS_LEN };
    enum InputId {
        SELECTIONS1_INPUT,
        SELECTIONS2_INPUT,
        SELECTIONS3_INPUT,
        SELECTIONS4_INPUT,
        SELECTIONS5_INPUT,
        SELECTIONS6_INPUT,
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId { LIGHTS_LEN };
    int const INPUTS_AND_OUTPUTS = 6;

    Coerce6()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(IN1_INPUT, "1");
        configInput(IN2_INPUT, "2");
        configInput(IN3_INPUT, "3");
        configInput(IN4_INPUT, "4");
        configInput(IN5_INPUT, "5");
        configInput(IN6_INPUT, "6");
        configInput(SELECTIONS1_INPUT, "Quantize 1");
        configInput(SELECTIONS2_INPUT, "Quantize 2");
        configInput(SELECTIONS3_INPUT, "Quantize 3");
        configInput(SELECTIONS4_INPUT, "Quantize 4");
        configInput(SELECTIONS5_INPUT, "Quantize 5");
        configInput(SELECTIONS6_INPUT, "Quantize 6");
        configOutput(OUT1_OUTPUT, "1");
        configOutput(OUT2_OUTPUT, "2");
        configOutput(OUT3_OUTPUT, "3");
        configOutput(OUT4_OUTPUT, "4");
        configOutput(OUT5_OUTPUT, "5");
        configOutput(OUT6_OUTPUT, "6");

        configBypass(IN1_INPUT, OUT1_OUTPUT);
        configBypass(IN2_INPUT, OUT2_OUTPUT);
        configBypass(IN3_INPUT, OUT3_OUTPUT);
        configBypass(IN4_INPUT, OUT4_OUTPUT);
        configBypass(IN5_INPUT, OUT5_OUTPUT);
        configBypass(IN6_INPUT, OUT6_OUTPUT);

        onReset(ResetEvent());
    }

    void process(const ProcessArgs& args) override
    {
        for (int i = 0; i < INPUTS_AND_OUTPUTS; i++) {
            if (inputs[IN1_INPUT + i].isConnected() && outputs[OUT1_OUTPUT + i].isConnected()) {
                int sel_count = 0;
                int sel_id = 0;
                std::array<float, 16> selections = {};
                outputs[OUT1_OUTPUT + i].setChannels(inputs[IN1_INPUT + i].getChannels());
                if (getNormalledSelections(i, sel_count, sel_id, selections)) {
                    adjustValues(sel_id, inputs[IN1_INPUT + i].getVoltages(),
                                 outputs[OUT1_OUTPUT + i].getVoltages(),
                                 inputs[IN1_INPUT + i].getChannels(), selections.data(), sel_count);
                }
                else {
                    outputs[OUT1_OUTPUT + i].setVoltage(inputs[IN1_INPUT + i].getVoltage());
                }
            }
        }
    }
};

struct Coerce1 : Coerce {
    Coerce1()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(IN1_INPUT, "1");
        configInput(SELECTIONS1_INPUT, "Quantize 1");
        configOutput(OUT1_OUTPUT, "1");
        configBypass(IN1_INPUT, OUT1_OUTPUT);

        onReset(ResetEvent());
    }

    void process(const ProcessArgs& /*args*/) override  // XXX This is double code. Can be improved.
    {
        for (int i = 0; i < INPUTS_AND_OUTPUTS; i++) {
            if (inputs[IN1_INPUT + i].isConnected() && outputs[OUT1_OUTPUT + i].isConnected()) {
                int sel_count = 0;
                int sel_id = 0;
                std::array<float, 16> selections = {};
                outputs[OUT1_OUTPUT + i].setChannels(inputs[IN1_INPUT + i].getChannels());
                if (getNormalledSelections(i, sel_count, sel_id, selections)) {
                    adjustValues(sel_id, inputs[IN1_INPUT + i].getVoltages(),
                                 outputs[OUT1_OUTPUT + i].getVoltages(),
                                 inputs[IN1_INPUT + i].getChannels(), selections.data(), sel_count);
                }
                else {
                    outputs[OUT1_OUTPUT + i].setVoltage(inputs[IN1_INPUT + i].getVoltage());
                }
            }
        }
    }
};

struct RestrictMethodMenuItem : MenuItem {
   private:
    Coerce* module;
    Coerce::RestrictMethod method;

   public:
    void onAction(const event::Action& e) override
    {
        module->restrictMethod = method;
    }
    void step() override
    {
        rightText = (module->restrictMethod == method) ? "✔" : "";
        MenuItem::step();
    }
    RestrictMethodMenuItem(std::string label, Coerce::RestrictMethod method, Coerce* module)
        : module(module), method(method)
    {
        this->text = std::move(label);
    }
};

struct RoundingMethodMenuItem : MenuItem {
   private:
    Coerce* module;
    Coerce::RoundingMethod method;

   public:
    void onAction(const event::Action& e) override
    {
        module->roundingMethod = method;
    }
    void step() override
    {
        rightText = (module->roundingMethod == method) ? "✔" : "";
        MenuItem::step();
    }
    RoundingMethodMenuItem(std::string label, Coerce::RoundingMethod method, Coerce* module)
        : module(module), method(method)
    {
        this->text = std::move(label);
    }
};
template <typename BASE, int PORTS>
struct CoerceWidget : ModuleWidget {
    explicit CoerceWidget(BASE* module)
    {
        setModule(module);
        std::string path = settings::preferDarkPanels ? "res/panels/dark/" : "res/panels/light/";
        std::string svg;
        if (std::is_same_v<BASE, Coerce1>) {
            svg = path + "Coerce.svg";
        }
        else {
            svg = path + "Coerce6.svg";
        }
        setPanel(createPanel(asset::plugin(pluginInstance, svg)));
        if (PORTS == 1) {
            addInput(createInputCentered<SIMPort>(mm2px(Vec(5.08, 40.0)), module, BASE::IN1_INPUT));
            addInput(createInputCentered<SIMPort>(mm2px(Vec(5.08, 55.0)), module,
                                                  BASE::SELECTIONS1_INPUT));
            addOutput(
                createOutputCentered<SIMPort>(mm2px(Vec(5.08, 70.0)), module, BASE::OUT1_OUTPUT));
        }
        else {
            for (int i = 0; i < PORTS; i++) {
                addInput(createInputCentered<SIMPort>(mm2px(Vec(5.08, 30.0 + i * 10.0)), module,
                                                      BASE::IN1_INPUT + i));
                addInput(createInputCentered<SIMPort>(mm2px(Vec(15.24, 30.0 + i * 10.0)), module,
                                                      BASE::SELECTIONS1_INPUT + i));
                addOutput(createOutputCentered<SIMPort>(mm2px(Vec(25.48, 30.0 + i * 10.0)), module,
                                                        BASE::OUT1_OUTPUT + i));
            }
        }
    };
    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Coerce*>(this->module);

        menu->addChild(new MenuSeparator);
        menu->addChild(
            new RestrictMethodMenuItem{"Octave Fold", BASE::RestrictMethod::OCTAVE_FOLD, module});
        menu->addChild(
            new RestrictMethodMenuItem{"Restrict", BASE::RestrictMethod::RESTRICT, module});
        menu->addChild(new MenuSeparator);
        menu->addChild(new RoundingMethodMenuItem{"Up", BASE::RoundingMethod::UP, module});
        menu->addChild(
            new RoundingMethodMenuItem{"Closest", BASE::RoundingMethod::CLOSEST, module});
        menu->addChild(new RoundingMethodMenuItem{"Down", BASE::RoundingMethod::DOWN, module});
        menu->addChild(new MenuSeparator);
    }
};

// const char svgMacro[] = "res/panels/Coerce6.svg";
// const char svgMicro[] = "res/panels/Coerce.svg";

Model* modelCoerce6 = createModel<Coerce6, CoerceWidget<Coerce6, 6>>("Coerce6");  // NOLINT
Model* modelCoerce = createModel<Coerce1, CoerceWidget<Coerce1, 1>>("Coerce");    // NOLINT
