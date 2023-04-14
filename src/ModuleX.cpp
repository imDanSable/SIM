#include "ModuleX.hpp"
#include <map>
using namespace std;


ModuleX::ModuleX(const ModelsListType& leftAllowedModels, const ModelsListType& rightAllowedModels,
    std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) :
    leftAllowedModels(leftAllowedModels), rightAllowedModels(rightAllowedModels), 
    leftLightOn(leftLightOn), rightLightOn(rightLightOn)
{
}
void ModuleX::onRemove(const RemoveEvent &e)
{
}
void ModuleX::process(const ProcessArgs &args)
{
		expanderUpdate |= expanderUpdateTimer.process(args.sampleTime) > XP_UPDATE_TIME;
		if (expanderUpdate)
		{
			expanderUpdate = false;
			updateLeftCachedExpanderModules();
			updateRightCachedExpanderModules();
		}
};

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
    const ModuleX* expander = e.side ? dynamic_cast<const ModuleX*>(rightExpander.module) : dynamic_cast<const ModuleX*>(leftExpander.module);
    std::function<void(float)> lightOn = e.side ? rightLightOn : leftLightOn;
    if (!expander)
    {
        lightOn(0.f);
        return;
    }
    const Model* expanderModel = expander->model;
    const ModelsListType& allowedModels = e.side ? rightAllowedModels : leftAllowedModels;
    for (const Model* model : allowedModels)
    {
        if (model == expanderModel)
        {
            lightOn(1.f);
            return;
        }
    }
}
