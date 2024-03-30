#pragma once
#include <string>
#include "constants.hpp"
#include "plugin.hpp"

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

template <typename TLight>
struct SIMLightLatch : VCVLightLatch<TLight> {
    SIMLightLatch() : themeInstance(&Themable::getInstance())
    {
        themeChange();
        this->momentary = false;
        this->latch = true;
        math::Vec svgSize = this->sw->box.size;
        this->box.size = svgSize;
        this->shadow->box.pos = math::Vec(0, 1.1 * svgSize.y);
        this->shadow->box.size = svgSize;
    }
    void step() override
    {
        VCVLightLatch<TLight>::step();
        math::Vec center = this->box.size.div(2);
        this->light->box.pos = center.minus(this->light->box.size.div(2));
        if (this->shadow) {
            // Update the shadow position to match the center of the button
            this->shadow->box.pos =
                center.minus(this->shadow->box.size.div(2).plus(math::Vec(0, -1.5F)));
        }
        themeChange();
    }
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            const std::string path = themeInstance->getComponentPath();
            this->frames.clear();
            this->addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMLightButton_0.svg")));
            this->addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMLightButton_1.svg")));
            this->sw->setSvg(this->frames[0]);
            this->fb->dirty = true;
            lastTheme = themeInstance->getCurrentTheme();
        }
    }

   private:
    Themable* themeInstance = &Themable::getInstance();
    int lastTheme = -1;
};
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

struct BaseDisplayWidget : TransparentWidget {
   private:
    friend struct LCDWidget;
    NVGcolor lcdColor = nvgRGB(0x21, 0x11, 0x11);
    NVGcolor lcdGhostColor = nvgRGBA(0xff, 0xaa, 0x11, 0x33);
    NVGcolor lcdTextColor = nvgRGB(0xff, 0xc3, 0x34);
    NVGcolor haloColor = lcdTextColor;

   public:
    void draw(const DrawArgs& args) override
    {
        drawBackground(args);
    }

    void drawBackground(const DrawArgs& args)
    {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 2.0, 2.0, box.size.x - 4.0, box.size.y - 4.0, 1.0);
        nvgFillColor(args.vg, lcdColor);
        nvgFill(args.vg);
    }
};

struct ModeSwitch : app::SvgSwitch {
    ModeSwitch() : themeInstance(&Themable::getInstance())
    {
        themeChange();
    }
    void step() override
    {
        SvgSwitch::step();
        themeChange();
    }

   private:
    void themeChange()
    {
        if (themeInstance->getCurrentTheme() != lastTheme) {
            const std::string path = themeInstance->getComponentPath();
            this->frames.clear();
            this->addFrame(
                Svg::load(asset::plugin(pluginInstance, path + "SIMTinyYellowLightSwitch.svg")));
            this->addFrame(
                Svg::load(asset::plugin(pluginInstance, path + "SIMTinyBlueLightSwitch.svg")));
            lastTheme = themeInstance->getCurrentTheme();
        }
    }
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};
/*
Gebruik LCDWidget
        auto* display = new LCDWidget();

        display->box.pos = Vec(35, 318);
        display->box.size = Vec(20, 21);
        display->offset = 1;
        display->textGhost = "18";
        if (module) {
            // make value point to an int pointer
            display->value = &module->prevEditChannel;
        }

        addChild(display);
*/
struct LCDWidget : BaseDisplayWidget {
    int* value = nullptr;          // NOLINT
    int offset = 0;                // NOLINT
    std::string textGhost = "88";  // NOLINT

    void drawLayer(const DrawArgs& args, int layer) override
    {
        if (layer != 1) { return; }

        std::shared_ptr<Font> font = APP->window->loadFont(
            asset::plugin(pluginInstance, "res/fonts/DSEG/DSEG7ClassicMini-Bold.ttf"));
        if (!font) { return; }

        nvgFontSize(args.vg, 11);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextLetterSpacing(args.vg, 1.0);
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);

        char integerString[10];                               // NOLINT
        snprintf(integerString, sizeof(integerString), "%d",  // NOLINT
                 value ? *value + offset : 1);                // NOLINT

        Vec textPos = Vec(box.size.x - 3.f, 16.0f);

        nvgFillColor(args.vg, lcdGhostColor);
        nvgText(args.vg, textPos.x, textPos.y, static_cast<const char*>(integerString), nullptr);

        NVGcolor fillColor = lcdTextColor;
        nvgFillColor(args.vg, fillColor);
        this->haloColor = fillColor;
        nvgText(args.vg, textPos.x, textPos.y, static_cast<const char*>(integerString), nullptr);

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
    }
};