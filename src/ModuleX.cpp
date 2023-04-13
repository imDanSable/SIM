#include "ModuleX.hpp"
#include <map>
using namespace std;
ModuleX::ModuleX()
{
}

void ModuleX::invalidateCachedModule(const Module* module)
{
    auto new_end = std::remove(connectedModules.begin(), connectedModules.end(), module);
    if (new_end != connectedModules.end())
    {
        // XXX TODO............................................. 
        // invalidate the pointer so process doesn't use it on the next frame
        connectedModules.erase(new_end, connectedModules.end());
    }
};

void ModuleX::onRemove(const RemoveEvent &e)
{
    if (masterModule)
    {
        // XXX TODO............................................. 
        masterModule->invalidateCachedModule(this);
    }
};

void ModuleX::onExpanderChange(const engine::Module::ExpanderChangeEvent &e)
{
		// this->updateTraversalCache(LEFT);
		// this->updateTraversalCache(RIGHT);
    sideType side = e.side ? RIGHT : LEFT;
    Module **lastModule = e.side ? &lastRightModule : &lastLeftModule;
    std::function<void(float)> lightOn = e.side ? rightLightOn : leftLightOn;
    ModuleX* toExp = side ? static_cast<ModuleX*>(&(*++ModuleX::ModuleIterator(this))) : static_cast<ModuleX*>(&(*--ModuleX::ModuleIterator(this)));
    bool turnlighton = side ? (++ModuleX::ModuleIterator(this) != ModuleX::ModuleIterator(nullptr)) : (--ModuleX::ModuleIterator(this) != ModuleX::ModuleIterator(nullptr));
    if (turnlighton) //&& isConnected(side))
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

bool ModelPtrLess(const Model *a, const Model *b)
{
    // XXX TODO FIX THIS. Too slow, and not needed
    return a->slug < b->slug;
}

ModuleX *ModuleX::addAllowedModel(Model *model, sideType side)
{
    ModelsListType &models = (side == LEFT) ? leftModels : rightModels;
    // ModelsListType::iterator it = std::lower_bound(models.begin(), models.end(), model, ModelPtrLess); // find proper position in descending order
    // models.insert(it, model);                                                                          // insert before iterator it
    // models.insert(model);
    models.emplace_back(model);
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
void ModuleX::updateTraversalCache(sideType side)
{
    ModuleTraverselCachType &traversalCache = (side == LEFT) ? leftTraversalCache : rightTraversalCache;
    traversalCache.clear();
    ModuleX::ModuleIterator it(this);
    while (it != ModuleX::ModuleIterator(nullptr))
    {
        traversalCache.push_back(&*it);
        ++it;
    }
    // ModuleX *curr = getNextConnected(side);

    // while (curr)
    // {
    //     traversalCache.push_back(curr);
    //     curr = curr->getNextConnected(side);
    // }
}

void ModuleX::structureChanged(sideType side)
{
    updateTraversalCache(side);
}

// Module *ModuleX::getModel(const Model *model, const sideType side)
// {
//     ModuleTraverselCachType &traversalCache = (side == LEFT) ? leftTraversalCache : rightTraversalCache;
//     // XXX TODO Not sure about == LEFT line below
//     const std::multimap<const Model *, const Model *> &consumeTable = (side == LEFT) ? leftConsumeTable : rightConsumeTable;

//     for (Module *curr : traversalCache)
//     {
//         // if (curr->beingRemoved)
//         // {
//         //     return nullptr;
//         // }

//         if (curr->isBypassed())
//         {
//             continue;
//         }

//         if (curr->model == model)
//         {
//             return curr;
//         }
//         auto range = consumeTable.equal_range(curr->model);
//         for (auto it = range.first; it != range.second; ++it)
//         {
//             if (it->second == model)
//             {
//                 return nullptr;
//             }
//         }
//     }
//     return nullptr;
// }
// ModuleX *ModuleX::getNextConnected(const sideType side)
// {
//     Expander &expander = (side == LEFT) ? leftExpander : rightExpander;
//     std::vector<Model *> &models = (side == LEFT) ? leftModels : rightModels;
//     if (expander.module == NULL)
//     {
//         return NULL;
//     }
//     for (Model *model : models)
//     {
//         if (model == expander.module->model)
//         {
//             return static_cast<ModuleX *>(expander.module);
//         }
//     }
//     return nullptr;
// }
