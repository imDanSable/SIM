#include "Connectable.hpp"
#include "ModuleX.hpp"
#include <vector>

void Connectable::checkLight(bool side, const Module *module,
                             const std::vector<Model *> &allowedModelss)
{
    std::function<void(float)> lightOn = side ? rightLightOn : leftLightOn;
    if (!module)
    {
        lightOn(0.F);
        return;
    }
    auto it = std::find(allowedModelss.begin(), allowedModelss.end(), module->model);
    lightOn(it != allowedModelss.end() ? 1.F : 0.F);
}
