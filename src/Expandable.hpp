#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "Connectable.hpp"
#include "constants.hpp"

template <typename T>
class Expandable : Connectable
{
public:
    Expandable(const ModelsListType& leftAllowedModels, const ModelsListType& rightAllowedModels,
        std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) :
        Connectable(leftLightOn, rightLightOn, leftAllowedModels, rightAllowedModels),
        module(dynamic_cast<T *>(this))
    {
    };

protected:
    T *module;

private:
    bool expanderUpdate = true;
    dsp::Timer expanderUpdateTimer;
};