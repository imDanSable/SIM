#pragma once
#include <rack.hpp>
#include "../plugin.hpp"

namespace comp {

struct SIMKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMKnob() : bg(new widget::SvgWidget), themeInstance(&Themable::getInstance())
    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        fb->addChildBelow(bg, tw);
        themeChange();
    }
    void step() override
    {
        ParamWidget::step();
        themeChange();
    }

   private:
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            const std::string path = themeInstance->getComponentPath();
            this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMKnob.svg")));
            bg->setSvg(
                APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMKnob-bg.svg")));
            lastTheme = themeInstance->getCurrentTheme();
        }
    }
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};

struct SIMSmallKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMSmallKnob() : bg(new widget::SvgWidget), themeInstance(&Themable::getInstance())

    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        fb->addChildBelow(bg, tw);
        themeChange();
    }
    void step() override
    {
        ParamWidget::step();
        themeChange();
    }

   private:
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            const std::string path = themeInstance->getComponentPath();
            this->setSvg(
                APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMSingleKnob.svg")));
            bg->setSvg(
                APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMSingleKnob-bg.svg")));
            lastTheme = themeInstance->getCurrentTheme();
        }
    }
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};
}  // namespace comp