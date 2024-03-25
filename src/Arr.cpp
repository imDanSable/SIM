// TODO: Finish paramsChanged (it is unaware of the changes in adapters)
// TODO: Make a user chosen rate limiter that is overriden by a trigger if applicable

#include <array>
#include <unordered_map>
#include "InX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "Segment.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/ParamQuantity.hpp"
#include "helpers.hpp"
#include "iters.hpp"
#include "plugin.hpp"

using iters::ParamIterator;

enum SnapTo { none, chromaticNotes, minorScale, majorScale, wholeVolts, tenSixteenth };
static const std::unordered_map<SnapTo, std::array<float, 8>> scales = {  // NOLINT
    {SnapTo::minorScale,
     {0.0f, 0.16666666666666666f, 0.25f, 0.4166666666666667f, 0.5833333333333334f,
      0.6666666666666666f, 0.8333333333333334f, 1.0f}},
    {SnapTo::majorScale,
     {0.0f, 0.16666666666666666f, 0.3333333333333333f, 0.4166666666666667f, 0.5833333333333334f,
      0.75f, 0.9166666666666666f, 1.0f}}};
struct Arr : public biexpand::Expandable<float> {
    enum ParamId {
        ENUMS(PARAM_KNOB, 16),
        PARAMS_LEN

    };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_MAIN, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

   private:
    int rootNote = 0;
    SnapTo snapTo{};

   public:
    struct ArrParamQuantity : ParamQuantity {
        static float quantizeValue(float value, SnapTo snapTo, int rootNote)
        {
            switch (snapTo) {
                case SnapTo::none: {
                    return value;
                }
                case SnapTo::wholeVolts: {
                    return std::round(value);
                }
                case SnapTo::tenSixteenth: {
                    return std::round(value * 1.6F) / (1.6F);
                }
                case SnapTo::chromaticNotes: {
                    return std::round(value * 12.F) / 12.F;
                }
                default: break;
            }

            // Assuming a scale
            auto voltages = scales.at(snapTo);
            value -= rootNote / 12.F;
            float minDistance = std::numeric_limits<float>::max();
            float closestVoltage = 0.0f;

            // Subtract the integer part
            int integerPart = static_cast<int>(value);
            float remainder = value - integerPart;

            // If the remainder is negative, add one
            bool addedOne = false;
            if (remainder < 0) {
                remainder += 1;
                addedOne = true;
            }

            // Find the closest note
            for (float scaleVoltage : voltages) {
                float distance = std::abs(remainder - scaleVoltage);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestVoltage = scaleVoltage;
                }
            }

            // Subtract one if we added one
            if (addedOne) { closestVoltage -= 1; }

            // Add the integer part back
            closestVoltage += integerPart;

            closestVoltage += rootNote / 12.F;

            return closestVoltage;
        }

       private:
       public:
        void setValue(float value) override
        {
            ParamQuantity::setImmediateValue(
                quantizeValue(value, reinterpret_cast<Arr*>(this->module)->snapTo,
                              reinterpret_cast<Arr*>(this->module)->rootNote));
        }

        // Display the value in the input box
        std::string getDisplayValueString() override
        {
            auto* module = reinterpret_cast<Arr*>(this->module);
            switch (module->snapTo) {
                case SnapTo::none: {
                    return string::f("%.3f", ParamQuantity::getValue());
                }
                case SnapTo::wholeVolts: {
                    return string::f("%.0f", ParamQuantity::getValue());
                }
                case SnapTo::tenSixteenth: {
                    return string::f(
                        "#%d", static_cast<int>(std::round(ParamQuantity::getValue() * 1.6F)));
                }
                default: {
                    return getNoteFromVoct(module->rootNote, module->snapTo == SnapTo::majorScale,
                                           std::round((ParamQuantity::getValue() * 12)));
                }
            }
        }
        // Input D#4 in input box
        void setDisplayValueString(std::string s) override
        {
            auto* module = reinterpret_cast<Arr*>(this->module);
            switch (module->snapTo) {
                case SnapTo::none: {
                    ParamQuantity::setDisplayValueString(s);
                    return;
                }
                case SnapTo::wholeVolts: {
                    ParamQuantity::setDisplayValueString(s);
                    const float value =
                        quantizeValue(ParamQuantity::getValue(), module->snapTo, module->rootNote);
                    module->params[this->paramId].setValue(value);
                    return;
                }
                case SnapTo::tenSixteenth: {
                    // Convert the string to #1, #2, #3, etc.
                    int number = 0;
                    std::string workString(s);
                    if (s[0] == '#') { workString = s.substr(1); }
                    try {
                        number = std::stoi(workString);
                    }
                    catch (...) {
                        ParamQuantity::setDisplayValueString(s);
                        const float value = quantizeValue(ParamQuantity::getValue(), module->snapTo,
                                                          module->rootNote);
                        module->params[this->paramId].setValue(value);
                        return;
                    }
                    const float value = number / 1.6F;
                    module->params[this->paramId].setValue(value);
                    return;
                }
                default: {
                    // Convert the string to notename
                    const float fromString = getVoctFromNote(s, NAN);
                    if (!std::isnan(fromString)) {
                        module->params[this->paramId].setValue(fromString);
                        return;
                    }
                    ParamQuantity::setDisplayValueString(s);
                    const float value =
                        quantizeValue(ParamQuantity::getValue(), module->snapTo, module->rootNote);
                    module->params[this->paramId].setValue(value);
                    return;
                }
            }
        }

        std::string getString() override
        {
            auto* module = reinterpret_cast<Arr*>(this->module);
            switch (module->snapTo) {
                case SnapTo::none: {
                    case SnapTo::wholeVolts:
                        return string::f("%s: %sV", ParamQuantity::getLabel().c_str(),
                                         ParamQuantity::getDisplayValueString().c_str());
                }
                case SnapTo::tenSixteenth: {
                    return string::f(
                        "%s: #%d", ParamQuantity::getLabel().c_str(),
                        static_cast<int>(std::round(ParamQuantity::getValue() * 1.6F)));
                }
                default: {
                    return getNoteFromVoct(module->rootNote, module->snapTo == SnapTo::majorScale,
                                           std::round((ParamQuantity::getValue() * 12)));
                }
            }
        }
    };  // ArrParamQuantity

   private:
    std::array<float, 16> paramsCache;
    friend struct ArrWidget;
    RexAdapter rex;
    InxAdapter inx;
    OutxAdapter outx;

    constants::VoltageRange voltageRange{constants::ZERO_TO_TEN};
    float minVoltage = 0.0F;
    float maxVoltage = 10.0F;

   public:
    Arr()
        : biexpand::Expandable<float>({{modelReX, &this->rex}, {modelInX, &this->inx}},
                                      {{modelOutX, &this->outx}})
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            configParam<ArrParamQuantity>(PARAM_KNOB + i, 0.0F, 10.F, 0.0F, "", "V");
        }
    }

    void performTransforms(bool forced = false)
    {
        bool changed = readVoltages(forced);
        // if (!changed && !forced) {
        //     for (auto* adapter : getLeftAdapters()) {
        //         if (adapter->isDirty()) {
        //             changed = true;
        //             break;
        //         }
        //     }
        //     for (auto* adapter : getRightAdapters()) {
        //         if (adapter->isDirty()) {
        //             changed = true;
        //             break;
        //         }
        //     }
        // }
        if (changed || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                perform_transform(*adapter);
            }
            if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
            perform_transform(outx);
            writeVoltages();
        }
    }
    void onUpdateExpanders(bool /*isRight*/) override
    {
        performTransforms(true);
    }
    void process(const ProcessArgs& /*args*/) override
    {
        performTransforms();
    }

    SnapTo getSnapTo() const
    {
        return snapTo;
    }

    void setSnapTo(SnapTo snapTo)
    {
        this->snapTo = snapTo;
        for (int param_id = PARAM_KNOB; param_id < PARAM_KNOB + 16; param_id++) {
            params[param_id].setValue(
                ArrParamQuantity::quantizeValue(params[param_id].getValue(), snapTo, rootNote));
        }
    }

    int getVoltageRange() const
    {
        return voltageRange;
    }
    void setVoltageRange(constants::VoltageRange range)
    {
        voltageRange = range;
        switch (voltageRange) {
            case constants::ZERO_TO_TEN:
                minVoltage = 0.0F;
                maxVoltage = 10.0F;
                break;
            case constants::ZERO_TO_FIVE:
                minVoltage = 0.0F;
                maxVoltage = 5.0F;
                break;
            case constants::ZERO_TO_THREE:
                minVoltage = 0.0F;
                maxVoltage = 3.0F;
                break;
            case constants::ZERO_TO_ONE:
                minVoltage = 0.0F;
                maxVoltage = 1.0F;
                break;
            case constants::MINUS_TEN_TO_TEN:
                minVoltage = -10.0F;
                maxVoltage = 10.0F;
                break;
            case constants::MINUS_FIVE_TO_FIVE:
                minVoltage = -5.0F;
                maxVoltage = 5.0F;
                break;
            case constants::MINUS_THREE_TO_THREE:
                minVoltage = -3.0F;
                maxVoltage = 3.0F;
                break;
            case constants::MINUS_ONE_TO_ONE:
                minVoltage = -1.0F;
                maxVoltage = 1.0F;
                break;
        }
        for (int i = PARAM_KNOB; i < PARAM_KNOB + constants::NUM_CHANNELS; i++) {
            const float old_value = getParam(i).getValue();
            ParamQuantity* pq = getParamQuantity(i);
            const float old_proportion = (old_value - pq->minValue) / (pq->maxValue - pq->minValue);
            pq->minValue = minVoltage;
            pq->maxValue = maxVoltage;
            getParam(i).setValue(pq->minValue + old_proportion * (pq->maxValue - pq->minValue));
            setSnapTo(snapTo);
        }
    }

    void onRandomize() override
    {
        for (int param_id = PARAM_KNOB; param_id < PARAM_KNOB + 16; param_id++) {
            const float value = random::uniform() * (maxVoltage - minVoltage) + minVoltage;
            params[param_id].setValue(ArrParamQuantity::quantizeValue(value, snapTo, rootNote));
        }
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "voltageRange", json_integer(static_cast<int>(voltageRange)));
        json_object_set_new(rootJ, "snapTo", json_integer(static_cast<int>(snapTo)));
        json_object_set_new(rootJ, "rootNote", json_integer(rootNote));
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            json_object_set_new(rootJ, ("knob" + std::to_string(i)).c_str(),
                                json_real(getParam(PARAM_KNOB + i).getValue()));
        }
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* voltageRangeJ = json_object_get(rootJ, "voltageRange");
        if (voltageRangeJ) {
            setVoltageRange(
                static_cast<constants::VoltageRange>(json_integer_value(voltageRangeJ)));
        }
        json_t* snapToJ = json_object_get(rootJ, "snapTo");
        if (snapToJ) { setSnapTo(static_cast<SnapTo>(json_integer_value(snapToJ))); }
        json_t* rootNoteJ = json_object_get(rootJ, "rootNote");
        if (rootNoteJ) { rootNote = json_integer_value(rootNoteJ); }
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            json_t* knobJ = json_object_get(rootJ, ("knob" + std::to_string(i)).c_str());
            if (knobJ) { getParam(PARAM_KNOB + i).setValue(json_real_value(knobJ)); }
        }
    }

   private:
    bool readVoltages(bool forced = false)
    {
        // This works still
        readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
        return true;
        bool changed{};
        if (!forced) {
            for (int i = 0; i < 16; i++) {  // XXX Better
                if (paramsCache[i] != params[i].getValue()) {
                    changed = true;
                    break;
                }
            }
        }
        if (changed || forced) {  // XXX Faster?
            for (int i = 0; i < 16; i++) {
                paramsCache[i] = params[i].getValue();
            }
            readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
        }

        return changed;
    }
    // bool readVoltages()
    // {
    //     std::vector<float> newBuffer(ParamIterator{params.begin()}, ParamIterator{params.end()});
    //     const bool changed = (newBuffer != prevBuffer);
    //     if (changed) {
    //         prevBuffer = newBuffer;
    //         readBuffer() = newBuffer;
    //         // readBuffer().swap(newBuffer);
    //         // prevBuffer.swap(newBuffer);
    //     }
    //     return changed;
    // }
    void writeVoltages()
    {
        outputs[OUTPUT_MAIN].channels = readBuffer().size();  // NOLINT
        outputs[OUTPUT_MAIN].writeVoltages(readBuffer().data());
    }
};

using namespace dimensions;  // NOLINT
struct ArrWidget : ModuleWidget {
    explicit ArrWidget(Arr* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/light/Array.svg"),
                             asset::plugin(pluginInstance, "res/panels/dark/Array.svg")));
        if (module) {
            module->addDefaultConnectionLights(this, Arr::LIGHT_LEFT_CONNECTED,
                                               Arr::LIGHT_RIGHT_CONNECTED);
        }

        addChild(createSegment2x8Widget<Arr>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> Segment2x8Data {
                if (module->rex) {
                    return Segment2x8Data{module->rex.getStart(), module->rex.getLength(), 16, -1};
                }
                return Segment2x8Data{0, 16, 16, -1};
            }));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createParamCentered<SIMSmallKnob>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    Arr::PARAM_KNOB + (i * 8) + j));
            }
        }
        addChild(createOutputCentered<SIMPort>(mm2px(Vec(3 * HP, LOW_ROW + JACKYSPACE - 7.F)),
                                               module, Arr::OUTPUT_MAIN));
    }

    void appendContextMenu(Menu* menu) override
    {
        auto* module = dynamic_cast<Arr*>(this->module);
        assert(module);  // NOLINT

        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);  // NOLINT
        // Add a menu option to select the root note (C, C#, D, until B.)
        std::vector<std::string> rootNoteLabels = {"C",       "C\u266F", "D",       "D\u266F",
                                                   "E",       "F",       "F\u266F", "G",
                                                   "G\u266F", "A",       "A\u266F", "B"};
        menu->addChild(createIndexSubmenuItem(
            "Root Note", rootNoteLabels, [module]() { return module->rootNote; },
            [module](int index) { module->rootNote = index; }));

        std::vector<std::string> snapToLabels = {"None",        "Chromatic (1V/12)", "Minor scale",
                                                 "Major scale", "Octave (1V)",       "10V/16Steps"};

        menu->addChild(createIndexSubmenuItem(
            "Snap to", snapToLabels, [module]() { return module->getSnapTo(); },
            [module](int index) { module->setSnapTo(static_cast<SnapTo>(index)); }));

        std::vector<std::string> voltageRangeLabels = {"0V-10V", "0V-5V", "0V-3V", "0V-1V",
                                                       "+/-10V", "+/-5V", "+/-3V", "+/-1V"};
        menu->addChild(createIndexSubmenuItem(
            "Voltage Range", voltageRangeLabels, [module]() { return module->getVoltageRange(); },
            [module](int index) {
                module->setVoltageRange(static_cast<constants::VoltageRange>(index));
            }));
    }
};

Model* modelArr = createModel<Arr, ArrWidget>("Arr");  // NOLINT