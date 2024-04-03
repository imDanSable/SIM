#include <iomanip>
#include <rack.hpp>

std::string getFractionalString(float value, int numerator, int denominator);
float getVoctFromNote(const std::string& noteName, float onErrorVal);
std::string getNoteFromVoct(int rootNote, bool majorScale, int noteNumber);

struct ClockTracker {
   private:
    int triggersPassed{};
    float timePassed = 0.0F;
    float avgPeriod = 0.5F;  // 120 BPM default
    bool periodDetected = false;

    rack::dsp::SchmittTrigger clockTrigger;

   public:
    void init(float avgPeriod = 0.5F);
    float getPeriod() const;
    bool isPeriodDetected() const;
    float getTimePassed() const;
    float getTimeFraction() const;
    bool process(float dt, float pulse);
};

template <typename T>
struct SliderQuantity : rack::Quantity {
   private:
    T* valueSrc = nullptr;
    T minVal;
    T maxVal;
    T defVal;
    std::string label = "Label";
    std::string unit = "Unit";
    int precision;

   public:
    SliderQuantity(T* valSrc,
                   T min,
                   T max,
                   T defVal,
                   std::string label,
                   std::string unit,
                   int precision)
        : valueSrc(valSrc),
          minVal(min),
          maxVal(max),
          defVal(defVal),
          label(std::move(label)),
          unit(std::move(unit)),
          precision(precision)
    {
    }
    void setValue(T value) override
    {
        *valueSrc = rack::math::clamp(value, getMinValue(), getMaxValue());
    }
    T getValue() override
    {
        return *valueSrc;
    }
    T getMinValue() override
    {
        return minVal;
    }
    T getMaxValue() override
    {
        return maxVal;
    }
    T getDefaultValue() override
    {
        return defVal;
    }
    T getDisplayValue() override
    {
        return getValue();
    }
    std::string getDisplayValueString() override
    {
        T val = getDisplayValue();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << val;
        return oss.str();
    }
    void setDisplayValue(T displayValue) override
    {
        setValue(displayValue);
    }
    std::string getLabel() override
    {
        return label;
    }
    std::string getUnit() override
    {
        return " " + unit;
    }
};