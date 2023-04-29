#include "ModuleX.hpp"
#include <map>
#include <utility>

ModuleX::ModuleX(bool isOutputExpander,
                 const ModelsListType& leftAllowedModels,
                 const ModelsListType& rightAllowedModels,
                 int leftLightId,
                 int rightLightId)
    : Connectable(leftLightId, rightLightId, leftAllowedModels, rightAllowedModels),
      isOutputExpander(isOutputExpander)

{
}

void ModuleX::onRemove()
{
    if (chainChangeCallback) {
        chainChangeCallback(ChainChangeEvent{});
        chainChangeCallback = nullptr;
    }
}
ModuleX::~ModuleX()
{
    DEBUG("ModuleX::~ModuleX()");
    // if (chainChangeCallback) { chainChangeCallback(ChainChangeEvent{}); }
};

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent& e)
{
    checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
               e.side ? rightAllowedModels : leftAllowedModels);
    if (chainChangeCallback) {
        chainChangeCallback(ChainChangeEvent{});
        if ((isOutputExpander && !e.side && !rightExpander.module) ||
            (!isOutputExpander && e.side && !leftExpander.module)) {
            chainChangeCallback = nullptr;
        };
    }
}
