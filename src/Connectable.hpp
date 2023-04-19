#pragma once
#include "plugin.hpp"
#include <functional>
#include <vector>


//XXX Perhaps use CRTP to have access to derived so checklight knows its derived 
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

	const ModelsListType leftAllowedModels;
	const ModelsListType rightAllowedModels;

private:
    std::function<void(float)> leftLightOn, rightLightOn;
};