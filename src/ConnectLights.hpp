#pragma once
#include "plugin.hpp"
#include <functional>
#include <vector>
// #include "ModuleX.hpp"


class ConnectLights
{
public:
    ConnectLights(std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) :
        leftLightOn(leftLightOn), rightLightOn(rightLightOn)
    {
    };
    void checkLight(bool side, const Module* module, const std::vector<Model *> &allowedModels);

private:
    std::function<void(float)> leftLightOn, rightLightOn;
};