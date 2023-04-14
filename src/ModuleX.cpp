#include "ModuleX.hpp"
#include <map>
using namespace std;


ModuleX::ModuleX(const ModelsListType& leftAllowedModels, const ModelsListType& rightAllowedModels,
    std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) :
    Connectable(leftLightOn, rightLightOn, leftAllowedModels, rightAllowedModels)
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
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module, e.side ? rightAllowedModels : leftAllowedModels);
}
