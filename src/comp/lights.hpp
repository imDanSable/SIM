#pragma once
#include <rack.hpp>
#include "../constants.hpp"
#include "../plugin.hpp"

namespace comp {

template <typename TBase = GrayModuleLightWidget>
struct TSIMConnectionLight : TBase {
    TSIMConnectionLight() : themeInstance(&Themable::getInstance())
    {
        themeChange();
    }
    void step() override
    {
        TBase::step();
        themeChange();
    }
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            this->baseColors.clear();
            switch (themeInstance->getCurrentTheme()) {
                case 0: this->addBaseColor(colors::panelPink); break;  // Vapor
                case 1: this->addBaseColor(SCHEME_GREEN); break;       // Dark
                case 2: this->addBaseColor(SCHEME_RED); break;         // Light
                default:
                    this->addBaseColor(SCHEME_GREEN);
                    assert(false);
                    break;
            }
            lastTheme = themeInstance->getCurrentTheme();
        }
    }

   private:
    Themable* themeInstance = &Themable::getInstance();
    int lastTheme = -1;
};
using SIMConnectionLight = TSIMConnectionLight<>;

}  // namespace comp