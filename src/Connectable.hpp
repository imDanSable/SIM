#pragma once
#include "plugin.hpp"
#include <functional>
#include <vector>


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
    /// @brief Call from derived class's onExpanderChange() method.
    void checkLight(bool side, const Module* module, const std::vector<Model *> &allowedModels);

protected:
	const ModelsListType leftAllowedModels = ModelsListType{modelReX, modelInX};
	const ModelsListType rightAllowedModels = ModelsListType{modelReX, modelSpike};

private:
    std::function<void(float)> leftLightOn, rightLightOn;
};