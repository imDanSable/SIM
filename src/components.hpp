#pragma once
#include <string>
#include "constants.hpp"
#include "plugin.hpp"

template <typename TLight>
struct SIMLightLatch : VCVLightLatch<TLight> {
    SIMLightLatch()
    {
        const std::string path =
            settings::preferDarkPanels ? "res/components/dark/" : "res/components/light/";
        this->momentary = false;
        this->latch = true;
        this->frames.clear();
        this->addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMLightButton_0.svg")));
        this->addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMLightButton_1.svg")));
        this->sw->setSvg(this->frames[0]);
        this->fb->dirty = true;
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
    }
};
struct SIMPort : app::SvgPort {
    SIMPort()
    {
        const std::string path = settings::preferDarkPanels ? "res/components/dark/SIMPort.svg"
                                                            : "res/components/light/SIMPort.svg";
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path)));
    }
};
struct SIMKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMKnob() : bg(new widget::SvgWidget)
    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        const std::string path =
            settings::preferDarkPanels ? "res/components/dark/" : "res/components/light/";
        fb->addChildBelow(bg, tw);
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMKnob.svg")));
        bg->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMKnob-bg.svg")));
    }
};

struct SIMEncoder : SIMKnob {
    void onEnter(const event::Enter& e) override {}
    void onLeave(const event::Leave& e) override {}
    void step() override
    {
        ParamWidget::step();
    }
};

struct SIMSmallKnob : SvgKnob {
    widget::SvgWidget* bg;  // NOLINT

    SIMSmallKnob() : bg(new widget::SvgWidget)
    {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        const std::string path =
            settings::preferDarkPanels ? "res/components/dark/" : "res/components/light/";
        fb->addChildBelow(bg, tw);
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMSingleKnob.svg")));
        bg->setSvg(
            APP->window->loadSvg(asset::plugin(pluginInstance, path + "SIMSingleKnob-bg.svg")));
    }
};

struct BaseDisplayWidget : TransparentWidget {
    NVGcolor backgroundColor = nvgRGB(0x04, 0x03, 0x01);
    NVGcolor lcdColor = nvgRGB(0x21, 0x11, 0x11);
    NVGcolor lcdGhostColor = nvgRGBA(0xff, 0xaa, 0x11, 0x33);
    NVGcolor lcdTextColor = nvgRGB(0xff, 0xc3, 0x34);
    NVGcolor haloColor = lcdTextColor;

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
    ModeSwitch()
    {
        const std::string path =
            settings::preferDarkPanels ? "res/components/dark/" : "res/components/light/";
        addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMTinyBlueLightSwitch.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, path + "SIMTinyPinkLightSwitch.svg")));
    }
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
    int* value = nullptr;
    int offset = 0;
    std::string textGhost = "88";

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

        char integerString[10];
        snprintf(integerString, sizeof(integerString), "%d", value ? *value + offset : 1);

        Vec textPos = Vec(box.size.x - 3.f, 16.0f);

        nvgFillColor(args.vg, lcdGhostColor);
        nvgText(args.vg, textPos.x, textPos.y, textGhost.c_str(), NULL);

        NVGcolor fillColor = lcdTextColor;
        nvgFillColor(args.vg, fillColor);
        this->haloColor = fillColor;
        nvgText(args.vg, textPos.x, textPos.y, integerString, NULL);

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
    }
};