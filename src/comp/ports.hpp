#pragma once
#include <rack.hpp>
#include "../plugin.hpp"

namespace comp {

struct SIMPort : app::SvgPort {
    SIMPort() : themeInstance(&Themable::getInstance())
    {
        themeChange();
    }
    void step() override
    {
        SvgPort::step();
        themeChange();
    }

   private:
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            const std::string path = themeInstance->getComponentPath();
            this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMPort.svg")));
            lastTheme = themeInstance->getCurrentTheme();
        }
    }
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};

}  // namespace comp