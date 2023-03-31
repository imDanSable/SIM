#pragma once
#include <string>
#include "plugin.hpp"

struct SIMPort : app::SvgPort {
  SIMPort() {
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMPort.svg")));
  }
};
struct SIMKnob : SvgKnob {
    widget::SvgWidget *bg;

  SIMKnob() {
    minAngle = -0.8 * M_PI;
    maxAngle = 0.8 * M_PI;
    bg = new widget::SvgWidget;
    fb->addChildBelow(bg, tw);
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob.svg")));
    bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob-bg.svg")));

  }
};
