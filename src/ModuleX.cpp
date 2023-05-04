#include "ModuleX.hpp"
#include <map>
#include <utility>
#include "Expandable.hpp"
#include "constants.hpp"
#include "plugin.hpp"

ModuleX::ModuleX(bool isOutputExpander, int leftLightId, int rightLightId)
    : Connectable(leftLightId, rightLightId), isOutputExpander(isOutputExpander)

{
}

void ModuleX::onRemove()
{
    DEBUG("ModuleX::onRemove()");  // NOLINT
    if (signalExpandable) {
        signalExpandable(ChainChangeEvent{});
        signalExpandable = nullptr;
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
    // if (e.side && !rightExpander.module) {
    //     setRightLightBrightness(0.F);
    // }
    // if (!e.side && !leftExpander.module) {
    //     setLeftLightBrightness(0.F);
    // }
    // assert(modulex);

    if (signalExpandable) {
        signalExpandable(ChainChangeEvent{});
        if ((isOutputExpander && !e.side && !rightExpander.module) ||
            (!isOutputExpander && e.side && !leftExpander.module)) {
            signalExpandable = nullptr;
        };
    }
}

template <typename T>
void BaseAdapter<T>::update(Expandable* expandable)
{
    this->setPtr(
        expandable->getExpander(modelReX, expandable->leftAllowedModels, constants::LEFT, this));
    // this->setPtr(expandable->getExpander<OutX, RIGHT>({modelOutX}));
}