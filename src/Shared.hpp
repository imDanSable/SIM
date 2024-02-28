#include "plugin.hpp"

struct ClockTracker {
   private:
    int triggersPassed{};
    float timePassed = 0.0F;
    float avgPeriod = 0.0F;
    bool periodDetected = false;

    dsp::SchmittTrigger clockTrigger;

   public:
    void init();
    float getPeriod() const;
    bool getPeriodDetected() const;
    float getTimePassed() const;
    float getTimeFraction() const;
    bool process(float dt, float pulse);
};