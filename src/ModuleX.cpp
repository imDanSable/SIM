#include "ModuleX.hpp"
#include <map>
#include <utility>

ModuleX::ModuleX(const ModelsListType &leftAllowedModels, const ModelsListType &rightAllowedModels,
                 std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn)
    : Connectable(std::move(leftLightOn), std::move(rightLightOn), leftAllowedModels,
                  rightAllowedModels)
{
}

ModuleX::~ModuleX()
{
    if (chainChangeCallback)
    {
        chainChangeCallback(ChainChangeEvent{});
    }
}

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
               e.side ? rightAllowedModels : leftAllowedModels);
    if (chainChangeCallback)
    {
        chainChangeCallback(ChainChangeEvent{});
    }
}
