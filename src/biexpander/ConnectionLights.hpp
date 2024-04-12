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
        using rack::math::Vec;
        using rack::window::mm2px;

        if (leftLightId != -1) {
            setLeftLightId(leftLightId);
            widget->addChild(rack::createLightCentered<rack::TinyLight<SIMConnectionLight>>(
                mm2px(Vec((x_offset), y_offset)), module, leftLightId));
            setLight(false, left);
        }

        if (rightLightId != -1) {
            setRightLightId(rightLightId);
            const float width_px = (widget->box.size.x);
            Vec vec = mm2px(Vec(-x_offset, y_offset));
            vec.x += width_px;
            widget->addChild(rack::createLightCentered<rack::TinyLight<SIMConnectionLight>>(
                vec, module, rightLightId));
            setLight(true, right);
        }
    }

    void setLight(bool isRight, bool state)
    {
        assert(module);
        if (isRight) {
            right = state;
            if (rightLightId != -1) {
                module->lights[rightLightId].setBrightness(state ? 1.0F : 0.0F);
            }
        }
        else if (!isRight) {
            left = state;
            if (leftLightId != -1) {
                module->lights[leftLightId].setBrightness(state ? 1.0F : 0.0F);
            }
        }
    }

   private:
    bool left{};
    bool right{};
    int leftLightId = -1;
    int rightLightId = -1;
    rack::engine::Module* module;
};