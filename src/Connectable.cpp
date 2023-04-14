#include "Connectable.hpp"
#include "ModuleX.hpp"
#include <vector>


void Connectable::checkLight(bool side, const Module* module, const std::vector<Model *> &allowedModels)
{
    const ModuleX *expander = dynamic_cast<const ModuleX *>(module);
    std::function<void(float)> lightOn = side ? rightLightOn : leftLightOn;
    if (!expander)
    {
        lightOn(0.f);
        return;
    }
    const Model *expanderModel = expander->model;
    for (const Model *model : allowedModels)
    {
        if (model == expanderModel)
        {
            lightOn(1.f);
            return;
        }
    }
    lightOn(0.f);
}
