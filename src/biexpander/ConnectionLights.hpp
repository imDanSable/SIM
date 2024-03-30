#include <rack.hpp>
#include "../components.hpp"  // XXX refactor out dependancy

class ConnectionLights {
   public:
    explicit ConnectionLights(rack::engine::Module* module) : module(module) {}
    void setLeftLightId(int id)
    {
        leftLightId = id;
    }

    void setRightLightId(int id)
    {
        rightLightId = id;
    }

    void addDefaultConnectionLights(rack::widget::Widget* widget,
                                    int leftLightId,
                                    int rightLightId,
                                    float x_offset = 1.7F,
                                    float y_offset = 2.F)
    {
        setLeftLightId(leftLightId);
        setRightLightId(rightLightId);

        using rack::math::Vec;
        using rack::window::mm2px;
        widget->addChild(rack::createLightCentered<rack::TinyLight<SIMConnectionLight>>(
            mm2px(Vec((x_offset), y_offset)), module, leftLightId));
        const float width_px = (widget->box.size.x);

        Vec vec = mm2px(Vec(-x_offset, y_offset));
        vec.x += width_px;
        widget->addChild(rack::createLightCentered<rack::TinyLight<SIMConnectionLight>>(
            vec, module, rightLightId));
        setLight(true, false);
        setLight(false, false);
    }

    void setLight(bool isRight, bool state)
    {
        if (isRight && rightLightId != -1) {
            module->lights[rightLightId].setBrightness(state ? 1.0F : 0.0F);
        }
        else if (!isRight && leftLightId != -1) {
            module->lights[leftLightId].setBrightness(state ? 1.0F : 0.0F);
        }
    }

   private:
    int leftLightId = -1;
    int rightLightId = -1;
    rack::engine::Module* module;
};