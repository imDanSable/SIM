#include <array>
#include <rack.hpp>
#include <unordered_map>
#include "InX.hpp"
#include "OutX.hpp"
#include "ReX.hpp"
#include "Shared.hpp"
#include "biexpander/biexpander.hpp"
#include "comp/Segment.hpp"
#include "comp/knobs.hpp"
#include "comp/ports.hpp"
#include "constants.hpp"
#include "helpers/iters.hpp"
#include "plugin.hpp"

using iters::ParamIterator;

enum SnapTo { none, chromaticNotes, minorScale, majorScale, wholeVolts, tenSixteenth, fractions };
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
    int numerator = 1;
    int denominator = 1;
    SnapTo snapTo{};

   public:
    float quantizeValue(float value /*, SnapTo snapTo, int rootNote*/)
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
            case SnapTo::fractions: {
                const float fraction = static_cast<float>(getNumerator()) / getDenominator();
                return std::round(value / fraction) * fraction;
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
    struct ArrParamQuantity : ParamQuantity {
       public:
        void setValue(float value) override
        {
            ParamQuantity::setImmediateValue(dynamic_cast<Arr*>(module)->quantizeValue(value));
        }

        // Display the value in the input box
        std::string getDisplayValueString() override
        {
            const auto* module = reinterpret_cast<const Arr*>(this->module);
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
                case SnapTo::fractions: {
                    return getFractionalString(ParamQuantity::getValue(), module->getNumerator(),
                                               module->getDenominator());
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
                    const float value = module->quantizeValue(ParamQuantity::getValue());
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
                        const float value = module->quantizeValue(ParamQuantity::getValue());
                        module->params[this->paramId].setValue(value);
                        return;
                    }
                    const float value = number / 1.6F;
                    module->params[this->paramId].setValue(value);
                    return;
                }
                default: {
                    // Convert the string to notename ..
                    const float fromString = getVoctFromNote(s, NAN);
                    if (!std::isnan(fromString)) {
                        // module->params[this->paramId].setValue(fromString);
                        const float value = module->quantizeValue(
                            clamp(fromString, module->minVoltage, module->maxVoltage));
                        module->params[this->paramId].setValue(value);
                        return;
                    }
                    ParamQuantity::setDisplayValueString(s);
                    const float value = module->quantizeValue(ParamQuantity::getValue());
                    module->params[this->paramId].setValue(value);
                    return;
                }
            }
        }

        std::string getString() override
        {
            const auto* const module = reinterpret_cast<Arr*>(this->module);
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
                case SnapTo::fractions: {
                    return getFractionalString(ParamQuantity::getValue(), module->getNumerator(),
                                               module->getDenominator());
                }
                default: {
                    return getNoteFromVoct(module->rootNote, module->snapTo == SnapTo::majorScale,
                                           std::round((ParamQuantity::getValue() * 12)));
                }
            }
        }
    };  // ArrParamQuantity

   private:
    friend struct ArrWidget;
    int cachedBufferSize = 0;
    /// @brief Read used by segment to get the buffer size
    int getBufferLength() const
    {
        return cachedBufferSize;

        // XXX options below result in flickers when swapping buffers.
        // return readBuffer().size();
        // return rex.getLength();
        // So we cache it
    }
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
        configCache();
    }
    void performTransforms(bool forced = false)  // 95% same as Bank
    {
        bool changed = readVoltages(forced);
        bool dirtyAdapters = false;
        if (!changed && !forced) { dirtyAdapters = this->dirtyAdapters(); }
        // Update our buffer because an adapter is dirty and our buffer needs to be updated
        if (!changed && !forced && dirtyAdapters) { readVoltages(true); }

        if (changed || dirtyAdapters || forced) {
            for (biexpand::Adapter* adapter : getLeftAdapters()) {
                transform(*adapter);
            }

            // Segment should reflex input changes but not output changes
            cachedBufferSize = readBuffer().size();

            if (outx) { outx.write(readBuffer().begin(), readBuffer().end()); }
            for (biexpand::Adapter* adapter : getRightAdapters()) {
                transform(*adapter);
            }
            writeVoltages();
        }
    }
    void onUpdateExpanders(bool /*isRight*/) override
    {
        // if it is inx, pass a lambda to quantize
        if (inx) {
            if (this->snapTo == SnapTo::none) { inx.setFloatValueFunction(nullptr); }
            else {
                inx.setFloatValueFunction([this](float value) { return quantizeValue(value); });
            }
        }
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
        quantizeAll();
        for (int param_id = PARAM_KNOB; param_id < PARAM_KNOB + 16; param_id++) {
            params[param_id].setValue(quantizeValue(params[param_id].getValue()));
        }
    }

    void setRootNote(int rootNote)
    {
        this->rootNote = rootNote;
        quantizeAll();
    }
    int getVoltageRange() const
    {
        return voltageRange;
    }
    void setVoltageRange(constants::VoltageRange range)
    {
        voltageRange = range;
        using constants::voltageRangeMinMax;
        minVoltage = voltageRangeMinMax[range].second.first;
        maxVoltage = voltageRangeMinMax[range].second.second;

        // This bit reads the relative values of the knobs and scales them to the new range
        for (int i = PARAM_KNOB; i < PARAM_KNOB + constants::NUM_CHANNELS; i++) {
            const float old_value = getParam(i).getValue();
            ParamQuantity* pq = getParamQuantity(i);
            const float old_proportion = (old_value - pq->minValue) / (pq->maxValue - pq->minValue);
            pq->minValue = minVoltage;
            pq->maxValue = maxVoltage;
            getParam(i).setValue(pq->minValue + old_proportion * (pq->maxValue - pq->minValue));
        }
        quantizeAll();
    }
    void setNumerator(int numerator)
    {
        this->numerator = numerator;
        this->snapTo = SnapTo::fractions;
        quantizeAll();
    }
    int getNumerator() const
    {
        return numerator;
    }
    void setDenominator(int denominator)
    {
        this->denominator = denominator;
        this->snapTo = SnapTo::fractions;
        quantizeAll();
    }
    int getDenominator() const
    {
        return denominator;
    }

    void quantizeAll()
    {
        for (int i = PARAM_KNOB; i < PARAM_KNOB + constants::NUM_CHANNELS; i++) {
            params[i].setValue(quantizeValue(clamp(params[i].getValue(), minVoltage, maxVoltage)));
        }
    }

    void onRandomize() override
    {
        for (int param_id = PARAM_KNOB; param_id < PARAM_KNOB + 16; param_id++) {
            const float value = random::uniform() * (maxVoltage - minVoltage) + minVoltage;
            params[param_id].setValue(quantizeValue(value));
        }
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "voltageRange", json_integer(static_cast<int>(voltageRange)));
        json_object_set_new(rootJ, "snapTo", json_integer(static_cast<int>(snapTo)));
        json_object_set_new(rootJ, "rootNote", json_integer(rootNote));
        json_object_set_new(rootJ, "numerator", json_integer(numerator));
        json_object_set_new(rootJ, "denominator", json_integer(denominator));
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
        json_t* numeratorJ = json_object_get(rootJ, "numerator");
        if (numeratorJ) { numerator = json_integer_value(numeratorJ); }
        json_t* denominatorJ = json_object_get(rootJ, "denominator");
        if (denominatorJ) { denominator = json_integer_value(denominatorJ); }
        for (int i = 0; i < constants::NUM_CHANNELS; i++) {
            json_t* knobJ = json_object_get(rootJ, ("knob" + std::to_string(i)).c_str());
            if (knobJ) { getParam(PARAM_KNOB + i).setValue(json_real_value(knobJ)); }
        }
    }

   private:
    bool readVoltages(bool forced = false)
    {
        const bool changed = this->cacheState.needsRefreshing();
        if (changed || forced) {
            readBuffer().assign(ParamIterator{params.begin()}, ParamIterator{params.end()});
            cacheState.refresh();
        }
        return changed;
    }

    void writeVoltages()
    {
        outputs[OUTPUT_MAIN].setChannels(readBuffer().size());
        outputs[OUTPUT_MAIN].writeVoltages(readBuffer().data());
    }
};

using namespace dimensions;  // NOLINT
struct ArrWidget : public SIMWidget {
    explicit ArrWidget(Arr* module)
    {
        setModule(module);
        setSIMPanel("Arr");

        if (module) {
            module->connectionLights.addDefaultConnectionLights(this, Arr::LIGHT_LEFT_CONNECTED,
                                                                Arr::LIGHT_RIGHT_CONNECTED);
        }

        addChild(comp::createSegment2x8Widget<Arr>(
            module, mm2px(Vec(0.F, JACKYSTART)), mm2px(Vec(4 * HP, JACKYSTART)),
            [module]() -> comp::SegmentData {
                if (module->rex) {
                    return comp::SegmentData{module->rex.getStart(), module->getBufferLength(), 16,
                                             -1};
                }
                return comp::SegmentData{0, 16, 16, -1};
            }));

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addParam(createParamCentered<comp::SIMSmallKnob>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    Arr::PARAM_KNOB + (i * 8) + j));
            }
        }
        addChild(createOutputCentered<comp::SIMPort>(mm2px(Vec(3 * HP, LOW_ROW + JACKYSPACE - 9.F)),
                                                     module, Arr::OUTPUT_MAIN));
    }

    void appendContextMenu(Menu* menu) override  // NOLINT
    {
        auto* module = dynamic_cast<Arr*>(this->module);
        assert(module);  // NOLINT

        SIMWidget::appendContextMenu(menu);

        menu->addChild(new MenuSeparator);
        menu->addChild(module->createExpandableSubmenu(this));
        menu->addChild(new MenuSeparator);

        std::vector<std::pair<std::string, constants::VoltageRange>> voltageRangeLabels = {
            {"0V-10V", constants::ZERO_TO_TEN},         {"0V-5V", constants::ZERO_TO_FIVE},
            {"0V-3V", constants::ZERO_TO_THREE},        {"0V-1V", constants::ZERO_TO_ONE},
            {"+/-10V", constants::MINUS_TEN_TO_TEN},    {"+/-5V", constants::MINUS_FIVE_TO_FIVE},
            {"+/-3V", constants::MINUS_THREE_TO_THREE}, {"+/-1V", constants::MINUS_ONE_TO_ONE}};

        menu->addChild(createSubmenuItem(
            "Voltage Range",
            [module, voltageRangeLabels]() -> std::string {
                for (const auto& pair : voltageRangeLabels) {
                    if (pair.second ==
                        module->getVoltageRange()) {  // cppcheck-suppress useStlAlgorithm
                        return pair.first;
                    }
                }
                return {};
            }(),
            [module, voltageRangeLabels](rack::Menu* menu) -> void {
                for (const auto& pair : voltageRangeLabels) {
                    auto* item = createMenuItem(
                        pair.first, (pair.second == module->getVoltageRange()) ? "✔" : "",
                        [module, range = pair.second]() { module->setVoltageRange(range); });
                    menu->addChild(item);
                }
            }));
        std::vector<std::pair<std::string, SnapTo>> scaleLabels = {
            {"Chromatic (1V/12)", SnapTo::chromaticNotes},
            {"Minor scale", SnapTo::minorScale},
            {"Major scale", SnapTo::majorScale},
            // Add more scales here...
        };
        std::vector<std::pair<std::string, SnapTo>> snapToLabels = {
            {"None", SnapTo::none},
            {"Octave (1V)", SnapTo::wholeVolts},
            {"Steps", SnapTo::tenSixteenth}};

        menu->addChild(createSubmenuItem(
            "Snap to",
            [module, snapToLabels, scaleLabels]() -> std::string {
                for (const auto& pair : snapToLabels) {
                    if (pair.second == module->snapTo) {  // cppcheck-suppress useStlAlgorithm
                        return pair.first;
                    }
                }
                for (const auto& pair : scaleLabels) {
                    if (pair.second == module->snapTo) {  // cppcheck-suppress useStlAlgorithm
                        return pair.first;
                    }
                }
                if (module->snapTo == SnapTo::fractions) {
                    return std::to_string(module->getNumerator()) + "/" +
                           std::to_string(module->getDenominator());
                }
                return {};
            }(),
            [module, snapToLabels, scaleLabels](rack::Menu* menu) -> void {
                for (const auto& pair : snapToLabels) {
                    auto* item = createMenuItem(
                        pair.first, (pair.second == module->snapTo) ? "✔" : "",
                        [module, snapTo = pair.second]() { module->setSnapTo(snapTo); });
                    menu->addChild(item);
                }
                auto* scalesMenu = createSubmenuItem(
                    "Scales", "", [module, scaleLabels](rack::Menu* menu) -> void {
                        for (const auto& pair : scaleLabels) {
                            auto* item = createMenuItem(
                                pair.first, (pair.second == module->snapTo) ? "✔" : "",
                                [module, snapTo = pair.second]() { module->setSnapTo(snapTo); });
                            menu->addChild(item);
                        }
                    });
                menu->addChild(scalesMenu);
                auto* fractionsMenu =
                    createSubmenuItem("Fractions", "", [module](rack::Menu* menu) -> void {
                        auto* item =
                            createSubmenuItem("Numerator", "", [module](rack::Menu* menu) -> void {
                                for (int i = 1; i <= constants::MAX_NUMERATOR; i++) {
                                    auto* item = createMenuItem(
                                        std::to_string(i), (i == module->getNumerator()) ? "✔" : "",
                                        [module, numerator = i]() {
                                            module->setNumerator(numerator);
                                        });
                                    menu->addChild(item);
                                }
                            });
                        menu->addChild(item);
                        item = createSubmenuItem(
                            "Denominator", "", [module](rack::Menu* menu) -> void {
                                // Add values 1 to 10
                                for (int i = 1; i <= constants::MAX_DENOMINATOR; i++) {
                                    auto* item =
                                        createMenuItem(std::to_string(i),
                                                       (i == module->getDenominator()) ? "✔" : "",
                                                       [module, denominator = i]() {
                                                           module->setDenominator(denominator);
                                                       });
                                    menu->addChild(item);
                                }
                            });
                        menu->addChild(item);
                    });
                menu->addChild(fractionsMenu);
            }));

        std::vector<std::pair<std::string, int>> rootNoteLabels = {
            {"C", 0}, {"C\u266F", 1}, {"D", 2}, {"D\u266F", 3},  {"E", 4}, {"F", 5}, {"F\u266F", 6},
            {"G", 7}, {"G\u266F", 8}, {"A", 9}, {"A\u266F", 10}, {"B", 11}};
        auto* rootNoteItem = createSubmenuItem(
            "Root Note",
            [module, rootNoteLabels]() -> std::string {
                for (const auto& pair : rootNoteLabels) {
                    if (pair.second == module->rootNote) {  // cppcheck-suppress useStlAlgorithm
                        return pair.first;
                    }
                }
                return {};
            }(),
            [module, rootNoteLabels](rack::Menu* menu) -> void {
                for (const auto& pair : rootNoteLabels) {
                    auto* item = createMenuItem(
                        pair.first, (pair.second == module->rootNote) ? "✔" : "",
                        [module, rootNote = pair.second]() { module->setRootNote(rootNote); });
                    menu->addChild(item);
                    if (module->snapTo != SnapTo::majorScale &&
                        module->snapTo != SnapTo::minorScale) {
                        item->disabled = true;
                    }
                }
            });
        if (module->snapTo != SnapTo::majorScale && module->snapTo != SnapTo::minorScale) {
            rootNoteItem->disabled = true;
        }
        menu->addChild(rootNoteItem);
    }
};

Model* modelArr = createModel<Arr, ArrWidget>("Arr");  // NOLINT