#include "ModuleX.hpp"
#include <map>
#include <utility>

ModuleX::ModuleX(bool isOutputExpander,
                 int leftLightId,
                 int rightLightId)
    : Connectable(leftLightId, rightLightId),
      isOutputExpander(isOutputExpander)

{
}

void ModuleX::onRemove()
{
    DEBUG("ModuleX::onRemove()");  // NOLINT
    if (chainChangeCallback) {
        chainChangeCallback(ChainChangeEvent{});
        chainChangeCallback = nullptr;
    }
}
ModuleX::~ModuleX()
{
    DEBUG("ModuleX::~ModuleX()");  // NOLINT
    // if (chainChangeCallback) { chainChangeCallback(ChainChangeEvent{}); }
};

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent& e)
{
    // checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
    //            e.side ? rightAllowedModels : leftAllowedModels);
    if (e.side && !rightExpander.module) {
        const int id = getRightLightId();
        if (id != -1) {
            lights[id].setBrightness(0.0F);
        }
    }
    if (!e.side && !leftExpander.module) {
        const int id = getLeftLightId();
        if (id != -1) {
            lights[id].setBrightness(0.0F);
        }
    }
    assert(modulex);

    if (chainChangeCallback) {
        chainChangeCallback(ChainChangeEvent{});
        if ((isOutputExpander && !e.side && !rightExpander.module) ||
            (!isOutputExpander && e.side && !leftExpander.module)) {
            chainChangeCallback = nullptr;
        };
    }
}
