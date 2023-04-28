#pragma once
#include <string>
#include "plugin.hpp"

struct SIMPort : app::SvgPort {
    SIMPort()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMPort.svg")));
    }
};
struct SIMKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMKnob() : bg(new widget::SvgWidget)
    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;

        fb->addChildBelow(bg, tw);
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob.svg")));
        bg->setSvg(
            APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SIMKnob-bg.svg")));
    }
};

struct SIMSingleKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMSingleKnob() : bg(new widget::SvgWidget)
    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;

        fb->addChildBelow(bg, tw);
        setSvg(APP->window->loadSvg(
            asset::plugin(pluginInstance, "res/components/SIMSingleKnob.svg")));
        bg->setSvg(APP->window->loadSvg(
            asset::plugin(pluginInstance, "res/components/SIMSingleKnob-bg.svg")));
    }
};
