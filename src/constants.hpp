#pragma once
#include "nanovg.h"

namespace dimensions
{
const float HP = 5.08F;
const float THREE_U = 128.500F;
const float JACKYSPACE = 7.5F;
const float JACKNTXT = 11.5F;     // Jack + text
const float PARAMJACKNTXT = 20.F; // Jack + text
const float JACKYSTART = 30.0F;
const float LOW_ROW = 103.0F;
const float Y_POSITION_CONNECT_LIGHT = 1.3F;
const float X_POSITION_CONNECT_LIGHT = 1.F;
} // namespace dimensions

namespace colors
{
const extern NVGcolor panelBgColor;
const extern NVGcolor panelPink;
const extern NVGcolor panelYellow;
const extern NVGcolor panelBlue;
const extern NVGcolor panelBrightBlue;
const extern NVGcolor panelBrightPink;
struct Color
{
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
} // namespace colors
namespace constants
{
static const int NUM_CHANNELS = 16;
static const int MAX_GATES = 16;
const float UI_UPDATE_TIME = 1.0F / 30.0F;
const float START_LEN_UPDATE_TIME = 10.F / 1.F;

enum sideType
{
    LEFT,
    RIGHT
};

constexpr inline sideType operator!(const sideType &side)
{
    return side == LEFT ? RIGHT : LEFT;
};

}; // namespace constants
