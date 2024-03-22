#include <rack.hpp>

float getVoctFromNote(const std::string& noteName, float onErrorVal);
std::string getNoteFromVoct(int rootNote, bool majorScale, int noteNumber);

struct ClockTracker {
   private:
    int triggersPassed{};
    float timePassed = 0.0F;
    float avgPeriod = 0.0F;
    bool periodDetected = false;

    rack::dsp::SchmittTrigger clockTrigger;

   public:
    void init(float avgPeriod = 0.0F);
    float getPeriod() const;
    bool getPeriodDetected() const;
    float getTimePassed() const;
    float getTimeFraction() const;
    bool process(float dt, float pulse);
};