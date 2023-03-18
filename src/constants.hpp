#pragma once
namespace constants
{

    enum sideType
    {
        LEFT,
        RIGHT
    };

    enum actionType
    {
        ADD,
        REMOVE
    };
    inline sideType operator!(sideType side)
    {
        return side == LEFT ? RIGHT : LEFT;
    };
    // static std::ostream &operator<<(std::ostream &os, const actionType &action)
    // {
    //     if (action == ADD)
    //     {
    //         os << "add";
    //     }
    //     else if (action == REMOVE)
    //     {
    //         os << "remove";
    //     }
    //     return os;
    // };

    // static std::ostream &operator<<(std::ostream &os, const sideType &side)
    // {
    //     if (side == LEFT)
    //     {
    //         os << "left";
    //     }
    //     else if (side == RIGHT)
    //     {
    //         os << "right";
    //     }
    //     return os;
    // }

    const float HP = 5.08f;
    const float JACKYSPACE = 7.5f;
    const float JACKNTXT = 11.5f; // Jack + text
    const float PARAMJACKNTXT = 20.f; // Jack + text
    const float JACKYSTART = 30.0f;
    const float LOW_ROW = 103.0f;
    const float Y_POSITION_CONNECT_LIGHT = 1.3f;
    const float X_POSITION_CONNECT_LIGHT = 1.f;
    static const int NUM_CHANNELS = 16;
    const float UI_UPDATE_TIME = 1.0f / 60.0f;
};