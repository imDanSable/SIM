#include "Shared.hpp"
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Define the note names for sharp and flat keys
const std::vector<std::string> sharpNoteNames = {"C",  "C♯", "D",  "D♯", "E",  "F",
                                                 "F♯", "G",  "G♯", "A",  "A♯", "B"};
const std::vector<std::string> flatNoteNames = {"C",  "D♭", "D",  "E♭", "E",  "F",
                                                "G♭", "G",  "A♭", "A",  "B♭", "B"};

// map of all note names and their corresponding voltages (1/12V per chromatic step)
const std::map<std::string, float> noteToVoltage = {
    {"C", 0.0F},        {"C#", 0.0833333F}, {"Db", 0.0833333F}, {"D", 0.1666667F},
    {"D#", 0.25F},      {"Eb", 0.25F},      {"E", 0.3333333F},  {"F", 0.4166667F},
    {"F#", 0.5F},       {"Gb", 0.5F},       {"G", 0.5833333F},  {"G#", 0.6666667F},
    {"Ab", 0.6666667F}, {"A", 0.75F},       {"A#", 0.8333333F}, {"Bb", 0.8333333F},
    {"B", 0.9166667F}};

float getVoctFromNote(const std::string& noteName, float onErrorVal)
{
    // Check if the note name is valid
    // One letter, optional sharp or flat, optional octave
    // Valid Letter?
    if (noteName.empty()) { return onErrorVal; }
    std::string note(1, noteName[0]);
    note[0] = std::toupper(note[0]);  // Convert to lower case
    if (!(note >= "A" && note <= "G")) { return onErrorVal; }
    // Optional sharp or flat
    std::string sharpFlat = noteName.substr(1, 1);
    if ((sharpFlat == "#" || sharpFlat == "b")) { note += sharpFlat; }
    else {
        sharpFlat = "";  // Clear the sharpFlat string
    }
    auto it = noteToVoltage.find(note);
    float noteVoltage = NAN;
    if (it != noteToVoltage.end()) { noteVoltage = it->second; }
    else {
        // handle error or return default value
        assert("Note not found in noteToVoltage map" && false);
        return onErrorVal;
    }
    // given the note name 'note' and the sharp/flat 'sharpFlat' calculate the correct voltage
    // knowing that c or C = 0 and that each step is 1/12 of a volt

    // Optional octave
    int octave = 0;
    std::string octaveStr = noteName.substr(1 + sharpFlat.size());
    char* end = nullptr;
    errno = 0;  // reset errno
    octave = std::strtol(octaveStr.c_str(), &end, 10);

    if (errno == ERANGE || *end != '\0') {
        // Conversion failed (out of range or invalid characters)
        return onErrorVal;
    }
    return noteVoltage + octave - 4;  // -4 because 0V is C4
}
/// @brief Get a string representation of a float value as a fraction
std::string getFractionalString(float value, int numerator, int denominator)
{
    int n = std::round(value * denominator / numerator);
    return std::to_string(n) + " * " + std::to_string(numerator) + "/" +
           std::to_string(denominator);
}
std::string getNoteFromVoct(int rootNote, bool majorScale, int noteNumber)
{
    // Calculate the note index
    int noteIndex = noteNumber % 12;
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