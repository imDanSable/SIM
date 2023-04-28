#include "Connectable.hpp"
#include <ratio>
#include <vector>
#include "ModuleX.hpp"

void Connectable::checkLight(bool side,
                             const Module* module,
                             const std::vector<Model*>& allowedModelss)
{
    if (!module) {
        lights[side ? rightLightId : leftLightId].value = 0.F;
        return;
    }
    auto it = std::find(allowedModelss.begin(), allowedModelss.end(), module->model);
    lights[side ? rightLightId : leftLightId].value = it != allowedModelss.end() ? 1.F : 0.F;
}

using namespace dimensions; // NOLINT

void Connectable::addConnectionLights(Widget* widget)
{
    widget->addChild(createLightCentered<TinySimpleLight<GreenLight>>(
        mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), this,
        leftLightId));
    const float width_px = (widget->box.size.x);

    Vec vec = mm2px(Vec(- X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT));
    vec.x += width_px;
    widget->addChild(createLightCentered<TinySimpleLight<GreenLight>>(vec, this, rightLightId));
}
