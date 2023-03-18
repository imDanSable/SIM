#include "ModuleX.hpp"
#include <map>
using namespace std;
ModuleX::ModuleX() //: leftMessages{SIM_ConnectMsg(nullptr), SIM_ConnectMsg(nullptr)},
                   //  rightMessages{SIM_ConnectMsg(nullptr), SIM_ConnectMsg(nullptr)}
{
    // rightExpander.producerMessage = &rightMessages[0];
    // rightExpander.consumerMessage = &rightMessages[1];
    // leftExpander.producerMessage = &leftMessages[0];
    // leftExpander.consumerMessage = &leftMessages[1];
}

void ModuleX::onRemove(const RemoveEvent &e)
{
    beingRemoved = true;
}
void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
    sideType side = e.side ? RIGHT : LEFT;
    Module **lastModule = e.side ? &lastRightModule : &lastLeftModule;
    std::function<void(float)> lightOn = e.side ? rightLightOn : leftLightOn;
    ModuleX *toExp = getNextConnected(side);
    if (toExp) //&& isConnected(side))
    {
        lightOn(1.f);
        *lastModule = toExp;
    }
    else
    {
        lightOn(0.f);
        if (*lastModule) // We had a module on this side
        {
            *lastModule = nullptr;
        }
    }
}

Module *ModuleX::getModel(const Model *model, const sideType side)
{
    auto *curr = this->getNextConnected(side);
    if (!curr)
        return nullptr;
    const pair<const Model *, const sideType> modelSearch = make_pair(curr->model, !side);
    while (curr)
    {
        if (curr->beingRemoved)
        {
            return nullptr;
        }
        // XXX TODO Not tested
        if (curr->isBypassed())
        {
            curr = curr->getNextConnected(side);
            continue;
        }
        if (curr->model == model)
            return curr;
        // XXX TODO Not tested
        if (this->consumeTable.find(modelSearch) != this->consumeTable.end())
            return nullptr;

        curr = curr->getNextConnected(side);
    }
    return nullptr;
}

ModuleX *ModuleX::addAllowedModel(Model *model, sideType side)
{
    if (side == LEFT)
        this->leftModels.emplace(model);
    else
        this->rightModels.emplace(model);
    return this;
}

ModuleX *ModuleX::setLeftLightOn(std::function<void(float)> lightOn)
{
    this->leftLightOn = lightOn;
    return this;
}

ModuleX *ModuleX::setRightLightOn(std::function<void(float)> lightOn)
{
    this->rightLightOn = lightOn;
    return this;
}

ModuleX *ModuleX::getNextConnected(const sideType side)
{
    if (side == LEFT)
    {
        if (leftExpander.module == NULL)
        {
            return NULL;
        }
        if (leftModels.find(leftExpander.module->model) != this->leftModels.end())
        {
            return dynamic_cast<ModuleX *>(leftExpander.module);
        }
    }
    else
    {
        if (rightExpander.module == NULL)
        {
            return NULL;
        }
        if (rightModels.find(rightExpander.module->model) != this->rightModels.end())
        {
            return dynamic_cast<ModuleX *>(rightExpander.module);
        }
    }
    return nullptr;
}


