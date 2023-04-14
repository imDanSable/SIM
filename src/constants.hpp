#pragma once
#include "nanovg.h"

namespace constants
{
    extern NVGcolor panelBgColor;
    extern NVGcolor panelPink;
    extern NVGcolor panelYellow;
    extern NVGcolor panelBlue;
    extern NVGcolor panelBrightBlue;
    extern NVGcolor panelBrightPink;
	struct Color
	{
		float r, g, b;
	};

	extern Color RED;
	extern Color GREEN;
	extern Color BLUE;
	extern Color YELLOW;
	extern Color CYAN;
	extern Color MAGENTA;
	extern Color WHITE;
	extern Color BLACK;
	extern Color GRAY;
	extern Color ORANGE;
	extern Color PURPLE;
	extern Color PINK;

    enum sideType
    {
        LEFT,
        RIGHT
    };

    inline sideType operator!(sideType side)
    {
        return side == LEFT ? RIGHT : LEFT;
    };
    

    const float HP = 5.08f;
    const float THREE_U = 128.500f;
    const float JACKYSPACE = 7.5f;
    const float JACKNTXT = 11.5f; // Jack + text
    const float PARAMJACKNTXT = 20.f; // Jack + text
    const float JACKYSTART = 30.0f;
    const float LOW_ROW = 103.0f;
    const float Y_POSITION_CONNECT_LIGHT = 1.3f;
    const float X_POSITION_CONNECT_LIGHT = 1.f;
    static const int NUM_CHANNELS = 16;
    const float UI_UPDATE_TIME = 1.0f / 30.0f;
    const float XP_UPDATE_TIME = 1.f / 10.f;


};

