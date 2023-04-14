#pragma once
#include "plugin.hpp"
// #include "../../include/plugin/Model.hpp"
#include <functional>
#include <vector>
// #include "ModuleX.hpp"


class Connectable
{
public:
	typedef std::vector<Model *> ModelsListType;
    Connectable(std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn,
        const ModelsListType& leftAllowedModels, const ModelsListType& rightAllowedModels) :
        leftAllowedModels(leftAllowedModels), rightAllowedModels(rightAllowedModels),
        leftLightOn(leftLightOn), rightLightOn(rightLightOn)
    {
    };
    void checkLight(bool side, const Module* module, const std::vector<Model *> &allowedModels);

protected:
	const ModelsListType leftAllowedModels = ModelsListType{modelReX, modelInX};
	const ModelsListType rightAllowedModels = ModelsListType{modelReX, modelSpike};

private:
    std::function<void(float)> leftLightOn, rightLightOn;
};