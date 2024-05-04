#include <rack.hpp>

namespace sp {

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

}  // namespace sp