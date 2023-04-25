#pragma once
#include "plugin.hpp"
#include <string>

struct SIMPort : app::SvgPort
{
  SIMPort() { setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMPort.svg"))); }
};
struct SIMKnob : SvgKnob
{
  widget::SvgWidget *bg; // NOLINT

  SIMKnob() : bg(new widget::SvgWidget)
  {
    minAngle = -0.8 * M_PI;
    maxAngle = 0.8 * M_PI;

    fb->addChildBelow(bg, tw);
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob.svg")));
    bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob-bg.svg")));
  }
};
