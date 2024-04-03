#pragma once
// #include "nanovg.h"
#include <array>
#include <rack.hpp>

namespace dimensions {
constexpr float HP = 5.08F;
const float THREE_U = 128.500F;
const float JACKYSPACE = 7.5F;
const float JACKNTXT = 11.5F;      // Jack + text
const float PARAMJACKNTXT = 20.F;  // Jack + text
const float JACKYSTART = 30.F;
const float LOW_ROW = 103.0F;
const float Y_POSITION_CONNECT_LIGHT = 1.3F;
const float X_POSITION_CONNECT_LIGHT = 1.F;
}  // namespace dimensions

namespace colors {
const extern NVGcolor panelBgColor;
const extern NVGcolor panelPink;
const extern NVGcolor panelSoftPink;
const extern NVGcolor panelYellow;
const extern NVGcolor panelBlue;
const extern NVGcolor panelBrightBlue;
const extern NVGcolor panelBrightPink;
const extern NVGcolor panelLightGray;
const extern NVGcolor panelDarkGray;
struct Color {
    float r, g, b;
} __attribute__((aligned(16)));

const extern Color RED;
const extern Color GREEN;
const extern Color BLUE;
const extern Color YELLOW;
const extern Color CYAN;
const extern Color MAGENTA;
const extern Color WHITE;
const extern Color BLACK;
const extern Color GRAY;
const extern Color ORANGE;
const extern Color PURPLE;
const extern Color PINK;
}  // namespace colors
namespace constants {

static const int NUM_CHANNELS = 16;
static const int MAX_GATES = 16;
static const int MAX_STEPS = 16;
constexpr float UI_UPDATE_TIME = 1.0F / 30.0F;  // (30FPS)
const float UI_UPDATE_DIVIDER = 256;
static const float BOOL_TRESHOLD = 0.5F;

enum sideType { LEFT, RIGHT };

constexpr inline sideType operator!(const sideType& side)
{
    return side == LEFT ? RIGHT : LEFT;
};

enum VoltageRange {
    ZERO_TO_TEN,
    ZERO_TO_FIVE,
    ZERO_TO_THREE,
    ZERO_TO_ONE,
    MINUS_TEN_TO_TEN,
    MINUS_FIVE_TO_FIVE,
    MINUS_THREE_TO_THREE,
    MINUS_ONE_TO_ONE
};

constexpr std::array<std::pair<VoltageRange, std::pair<float, float>>, 8> voltageRangeMinMax = {{
    {ZERO_TO_TEN, {0.0F, 10.0F}},
    {ZERO_TO_FIVE, {0.0F, 5.0F}},
    {ZERO_TO_THREE, {0.0F, 3.0F}},
    {ZERO_TO_ONE, {0.0F, 1.0F}},
    {MINUS_TEN_TO_TEN, {-10.0F, 10.0F}},
    {MINUS_FIVE_TO_FIVE, {-5.0F, 5.0F}},
    {MINUS_THREE_TO_THREE, {-3.0F, 3.0F}},
    {MINUS_ONE_TO_ONE, {-1.0F, 1.0F}},
}};

const uint8_t MAX_NUMERATOR = 15;
const uint8_t MAX_DENOMINATOR = 16;
}  // namespace constants