#pragma once
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
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
  int getStart(int channel = 0);
  int getLength(int channel = 0);


};

//XXX
class RexAble { // interface for Expandables that allow Rex to be attached

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

    addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, ReX::LIGHT_LEFT_CONNECTED));
    addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(width - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, ReX::LIGHT_RIGHT_CONNECTED));

    addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT)), module, ReX::PARAM_START));
    addInput(createInputCentered<SIMPort>(mm2px(Vec(center, JACKYSTART + 0 * PARAMJACKNTXT + JACKYSPACE)), module, ReX::INPUT_START));

    addParam(createParamCentered<SIMKnob>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT)), module, ReX::PARAM_LENGTH));
    addInput(createInputCentered<SIMPort>(mm2px(Vec(center, JACKYSTART + 1 * PARAMJACKNTXT + JACKYSPACE)), module, ReX::INPUT_LENGTH));
  }
};
