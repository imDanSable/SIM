#include "Shared.hpp"

#include <map>
#include <string>
#include <vector>

// Define the note names for sharp and flat keys
const std::vector<std::string> sharpNoteNames = {"C",  "C♯", "D",  "D♯", "E",  "F",

                                                 "F♯", "G",  "G♯", "A",  "A♯", "B"};
const std::vector<std::string> flatNoteNames = {"C",  "D♭", "D",  "E♭", "E",  "F",
                                                "G♭", "G",  "A♭", "A",  "B♭", "B"};

// Define the major and minor scale intervals
const std::vector<int> majorIntervals = {2, 2, 1, 2, 2, 2, 1};
const std::vector<int> minorIntervals = {2, 1, 2, 2, 1, 2, 2};

// Define a map of scales

std::string getCtxNoteName(int rootNote, bool majorScale, int noteNumber)
{
    // Get the scale intervals
    std::vector<int> intervals = majorScale ? majorIntervals : minorIntervals;

    // Calculate the note index
    int noteIndex = rootNote;
    for (int i = 0; i < noteNumber; ++i) {
        noteIndex += intervals[i % intervals.size()];
        noteIndex %= 12;
    }
    int octave = noteNumber / 12;

    // Determine whether to use sharp or flat note names
    std::vector<std::string> noteNames;
    if (majorScale && (rootNote == 0 || rootNote == 2 || rootNote == 4 || rootNote == 7 ||
                       rootNote == 9 || rootNote == 11)) {
        noteNames = sharpNoteNames;
    }
    else if (!majorScale && (rootNote == 2 || rootNote == 3 || rootNote == 5 || rootNote == 7 ||
                             rootNote == 8 || rootNote == 10)) {
        noteNames = flatNoteNames;
    }
    else {
        noteNames = sharpNoteNames;
    }

    // Return the note name
    return noteNames[noteIndex] + std::to_string(octave);
}
void ClockTracker::init(float avgPeriod)
{
    triggersPassed = 0;
    this->avgPeriod = avgPeriod;
    timePassed = 0.0F;
    if (avgPeriod > 0.0F) { periodDetected = true; }
}

float ClockTracker::getPeriod() const
{
    return avgPeriod;
}

bool ClockTracker::getPeriodDetected() const
{
    return periodDetected;
}

float ClockTracker::getTimePassed() const
{
    return timePassed;
}

float ClockTracker::getTimeFraction() const
{
    return timePassed / avgPeriod;
}

bool ClockTracker::process(const float dt, const float pulse)
{
    timePassed += dt;
    if (!clockTrigger.process(pulse)) { return false; }
    if (triggersPassed < 3) { triggersPassed += 1; }
    if (triggersPassed > 2) {
        periodDetected = true;
        avgPeriod = timePassed;
    }
    timePassed = 0.0F;
    return true;
};