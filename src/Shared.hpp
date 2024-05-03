#pragma once
#include <iomanip>
#include <limits>
#include <rack.hpp>

std::string getFractionalString(float value, int numerator, int denominator);
float getVoctFromNote(const std::string& noteName, float onErrorVal);
std::string getNoteFromVoct(int rootNote, bool majorScale, int noteNumber);

/**
 * Wraps a value into a specified range.
 *
 * This function takes a value `v` and two range boundaries `lo` and `hi`, and adjusts `v` so that
 * it falls within the range `[lo, hi)`. If `v` is outside the range, it will be adjusted by adding
 * or subtracting multiples of the range length (`hi - lo`) until it falls within the range. The
 * relative distance of `v` from `lo` or `hi` will be maintained.
 *
 * @param v The value to be wrapped.
 * @param lo The lower boundary of the range.
 * @param hi The upper boundary of the range.
 * @return The wrapped value, which will be in the range `[lo, hi)`.
 */
inline float wrap(float v, float lo, float hi)
{
    assert(lo < hi && "Calling wrap with wrong arguments");
    float range = hi - lo;
    float result = rack::eucMod(v - lo, range);
    return result + lo;
}

template <typename T>
inline T limit(T v, T max)
{
    return std::min(v, max - std::numeric_limits<T>::epsilon());
}


/// @brief returns the fractional part of a float
inline float frac(float v)
{
    return v - std::floor(v);
};
struct ClockTracker {
   private:
    int triggersPassed{};
    float timePassed = 0.0F;
    float avgPeriod = 0.5F;  // 120 BPM default if you ignore period detection
    bool periodDetected = false;

    rack::dsp::SchmittTrigger clockTrigger;

   public:
    void init(float avgPeriod = NAN);
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