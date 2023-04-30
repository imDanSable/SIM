#pragma once
#include <string>
#include "constants.hpp"
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

struct SIMEncoder : SIMKnob {
    void onEnter(const event::Enter& e) override {}
    void onLeave(const event::Leave& e) override {}
    void step() override
    {
        ParamWidget::step();
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

struct BaseDisplayWidget : TransparentWidget {

    NVGcolor backgroundColor = nvgRGB(0x04, 0x03, 0x01);
    NVGcolor lcdColor = nvgRGB(0x21, 0x11, 0x11);
    NVGcolor lcdGhostColor = nvgRGBA(0xff, 0xaa, 0x11, 0x25);
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