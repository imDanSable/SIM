#include <iomanip>
#include <rack.hpp>

namespace helpers {

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

}  // namespace helpers