#pragma once
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "engine/Module.hpp"
#include "plugin.hpp"

struct ReX : public ModuleX
{
    enum ParamId
    {
        PARAM_START,
        PARAM_LENGTH,
        PARAMS_LEN
    };
    enum InputId
    {
        INPUT_START,
        INPUT_LENGTH,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUTPUTS_LEN
    };
    enum LightId
    {
        LIGHT_LEFT_CONNECTED,
        LIGHT_RIGHT_CONNECTED,
        LIGHTS_LEN
    };

    ReX();
};

// XXX

template <class T> class RexAble
{ // interface for Expandables that allow Rex to be attached
  public:
    explicit RexAble(T *module) : module(module){};

    explicit operator bool() const
    {
        return rex != nullptr;
    }
    explicit operator ReX *() const
    {
        return rex;
    }

    ReX *operator->() const
    {
        return rex;
    }

    void setReX(ReX *rex)
    {
        this->rex = rex;
    }

    bool cvStartConnected()
    {
        return rex && rex->inputs[ReX::INPUT_START].isConnected();
    }

    bool cvLengthConnected()
    {
        return rex && rex->inputs[ReX::INPUT_LENGTH].isConnected();
    }

    int getLength(int channel, int max = constants::NUM_CHANNELS) const
    {
        if (!rex)
        {
            return constants::NUM_CHANNELS;
        }
        if (!rex->inputs[ReX::INPUT_LENGTH].isConnected())
        {

            return clamp(static_cast<int>(rex->params[ReX::PARAM_LENGTH].getValue()), 1, max);
        }

        return clamp(static_cast<int>(rescale(rex->inputs[ReX::INPUT_LENGTH].getVoltage(channel), 0,
                                              10, 1, static_cast<float>(max + 1))),
                     1, max);
    };
    int getStart(int channel, int max = constants::NUM_CHANNELS) const
    {
        if (!rex)
        {
            return 0;
        }
        if (!rex->inputs[ReX::INPUT_START].isConnected())
        {
            return clamp(static_cast<int>(rex->params[ReX::PARAM_START].getValue()), 0, max - 1);
        }
        return clamp(static_cast<int>(rescale(rex->inputs[ReX::INPUT_START].getPolyVoltage(channel),
                                              0, 10, 0, static_cast<float>(max))),
                     0, max - 1);
    };

  private:
    T *module;
    ReX *rex = nullptr;
};

using namespace dimensions; // NOLINT
struct ReXWidget : ModuleWidget
{
    explicit ReXWidget(ReX *module)
    {
        const float center = 1.F * HP;
        const float width = 2.F * HP;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Rex.svg")));

        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module,
            ReX::LIGHT_LEFT_CONNECTED));
        addChild(createLightCentered<TinySimpleLight<GreenLight>>(
            mm2px(Vec(width - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module,
            ReX::LIGHT_RIGHT_CONNECTED));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT)),
                                              module, ReX::PARAM_START));
        addInput(createInputCentered<SIMPort>(
            mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT + JACKYSPACE)), module,
            ReX::INPUT_START));

        addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT)),
                                              module, ReX::PARAM_LENGTH));
        addInput(createInputCentered<SIMPort>(
            mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT + JACKYSPACE)), module,
            ReX::INPUT_LENGTH));
    }
};
