#include "ModuleX.hpp"
#include <map>
using namespace std;


ModuleX::ModuleX(const ModelsListType& leftAllowedModels, const ModelsListType& rightAllowedModels,
    std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) :
    Connectable(leftLightOn, rightLightOn, leftAllowedModels, rightAllowedModels)
{
}

ModuleX::~ModuleX()
{
    if (chainChangeCallback)
        chainChangeCallback(ChainChangeEvent{}); 
}

// XXX Templatize left/right
void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
    //XXX feels like this should be moved to connectable
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module, e.side ? rightAllowedModels : leftAllowedModels);
    if (chainChangeCallback)
        chainChangeCallback(ChainChangeEvent{});
}
