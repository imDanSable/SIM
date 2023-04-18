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
    //DEBUG("Model: %s, ModuleX::~ModuleX()", model->name.c_str());
    if (chainChangeCallback)
        chainChangeCallback(ChainChangeEvent{}); 
}

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
    //DEBUG("Model: %s, ModuleX::onExpanderChange(%s)", model->name.c_str(), e.side ? "right" : "left");
    //XXX feels like this should be moved to connectable
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module, e.side ? rightAllowedModels : leftAllowedModels);
    if (chainChangeCallback)
        chainChangeCallback(ChainChangeEvent{});
}
