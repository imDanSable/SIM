#include "ModuleX.hpp"
#include <map>
#include <utility>

ModuleX::ModuleX(const ModelsListType& leftAllowedModels,
                 const ModelsListType& rightAllowedModels,
                 int leftLightId,
                 int rightLightId)
    : Connectable(leftLightId, rightLightId, leftAllowedModels, rightAllowedModels)
{
}

ModuleX::~ModuleX()
{
    if (chainChangeCallback) { chainChangeCallback(ChainChangeEvent{}); }
}

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent& e)
{
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
               e.side ? rightAllowedModels : leftAllowedModels);
    if (chainChangeCallback) { chainChangeCallback(ChainChangeEvent{}); }
}
