#pragma once
#include <rack.hpp>
#include "../plugin.hpp"

namespace comp {

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

struct BaseSwitch : SvgSwitch {
    // NVGcolor haloColor = nvgRGBA(0xff, 0xff, 0xff, 0xff);

    void drawHalo(const DrawArgs& args)
    {
        // Don't draw halo if rendering in a framebuffer, e.g. screenshots or Module Browser
        if (args.fb) { return; }

        const float halo = settings::haloBrightness;
        if (halo == 0.f) { return; }

        math::Vec c = box.size.div(2);
        float oradius = box.size.x * 1.5f;
        nvgBeginPath(args.vg);

        nvgCircle(args.vg, c.x, c.y, oradius);
        // Set inner color to white with alpha scaled by halo
        // But not 255 so let some color seep through.
        NVGcolor icol = nvgRGBA(0xff, 0xff, 0xff, halo * 200);
        NVGcolor ocol = nvgRGBA(0xff, 0xff, 0xff, 0x0);  // Set outer color to transparent white
        NVGpaint paint = nvgRadialGradient(args.vg, c.x, c.y, 0, oradius, icol, ocol);
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);
    }

    void draw(const DrawArgs& args) override
    {
        SvgSwitch::draw(args);
        drawHalo(args);
    }
};

struct ModeSwitch : BaseSwitch {
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

struct TriModeSwitch : BaseSwitch {
    TriModeSwitch() : themeInstance(&Themable::getInstance())
    {
        themeChange();
    }

    void onChange(const ChangeEvent& e) override
    {
        SvgSwitch::onChange(e);
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
            this->addFrame(
                Svg::load(asset::plugin(pluginInstance, path + "SIMTinyPinkLightSwitch.svg")));
            lastTheme = themeInstance->getCurrentTheme();
        }
    }
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};
}  // namespace comp