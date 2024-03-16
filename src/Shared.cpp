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
    // Calculate the note index
    int roundedNumber = std::round(noteNumber);
    int noteIndex = roundedNumber % 12;
    if (noteIndex < 0) {
        noteIndex += 12;  // Ensure noteIndex is between 0 and 11
    }

    // Calculate the octave
    int octave = noteNumber / 12;
    if (noteNumber < 0 && noteNumber % 12 != 0) {
        octave -= 1;  // Decrease the octave for negative noteNumber values not divisible by 12
    }
    octave += 4;  // Offset the octave to start from C4

    // Determine whether to use sharp or flat note names
    std::vector<std::string> noteNames;
    // C, D, E, G, A, B, F#, C#
    if (majorScale) {
        if (rootNote == 0 || rootNote == 2 || rootNote == 4 || rootNote == 7 || rootNote == 9 ||
            rootNote == 11 || rootNote == 6 || rootNote == 1) {
            noteNames = sharpNoteNames;
        }
        else {
            noteNames = flatNoteNames;
        }
    }
    else {  // Minor scale // C, D, E♭, F, G, A♭, B♭
        if ((rootNote == 0 || rootNote == 2 || rootNote == 3 || rootNote == 5 || rootNote == 7 ||
             rootNote == 8 || rootNote == 10)) {
            noteNames = flatNoteNames;
        }
        else {
            noteNames = sharpNoteNames;
        }
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