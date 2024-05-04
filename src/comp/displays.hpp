#pragma once
#include <rack.hpp>
#include "../plugin.hpp"

namespace comp {

// NOLINTBEGIN
struct BaseDisplayWidget : rack::TransparentWidget {
    friend struct LCDWidget;
    NVGcolor lcdColor = nvgRGB(0x21, 0x11, 0x11);
    NVGcolor backgroundColor = nvgRGB(0x01, 0x01, 0x01);
    NVGcolor lcdGhostColor = nvgRGBA(0xff, 0xaa, 0x11, 0x33);
    NVGcolor lcdTextColor = nvgRGB(0xff, 0xc3, 0x34);
    NVGcolor haloColor = nvgRGB(0xff, 0xc3, 0x2f);

    void draw(const DrawArgs& args) override
    {
        drawBackground(args);
    }
    void drawBackground(const DrawArgs& args)
    {
        // Background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0.f, 3.0, box.size.x, box.size.y + 3, 2.F);
        nvgFillColor(args.vg, backgroundColor);
        nvgFill(args.vg);

        // LCD
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 1.0f, 4.F, box.size.x - 2.f, box.size.y + 1.f, 2.0f);
        nvgFillColor(args.vg, lcdColor);
        nvgFill(args.vg);

        // nvgBeginPath(args.vg);
        // nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x , box.size.y - 7.F, 1.0);
        // nvgFillColor(args.vg, lcdColor);
        // nvgFill(args.vg);
    }

    void drawHalo(const DrawArgs& args)
    {
        // Don't draw halo if rendering in a framebuffer, e.g. screenshots or Module Browser
        if (args.fb) return;

        const float halo = rack::settings::haloBrightness;
        if (halo == 0.f) return;

        // If light is off, rendering the halo gives no effect.
        if (this->lcdTextColor.r == 0.f && this->lcdTextColor.g == 0.f &&
            this->lcdTextColor.b == 0.f)
            return;

        rack::math::Vec c = box.size.div(2);
        float radius = 0.f;
        float oradius = std::max(box.size.x, box.size.y);

        nvgBeginPath(args.vg);
        nvgRect(args.vg, c.x - oradius, c.y - oradius, 2 * oradius, 2 * oradius);

        float ourHalo = std::min(halo * 0.5f, 0.15f);
        NVGcolor icol = rack::color::mult(this->haloColor, ourHalo);
        NVGcolor ocol = nvgRGBA(this->haloColor.r, this->haloColor.g, this->haloColor.b, 0);
        // NVGcolor ocol = nvgRGBA(0xff, 0x00, 0x00, 0x1f);
        NVGpaint paint = nvgRadialGradient(args.vg, c.x, c.y, radius, oradius, icol, ocol);
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);
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
    int* value = nullptr;          // NOLINT
    int offset = 0;                // NOLINT
    std::string textGhost = "88";  // NOLINT

    void drawLayer(const DrawArgs& args, int layer) override
    {
        if (layer != 1) { return; }

        std::shared_ptr<rack::Font> font = APP->window->loadFont(
            rack::asset::plugin(pluginInstance, "res/fonts/DSEG/DSEG7ClassicMini-Bold.ttf"));
        if (!font) { return; }

        nvgFontSize(args.vg, 11);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextLetterSpacing(args.vg, 1.0);
        // nvgTextLineHeight(args.vg, 0.5f);
        // nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
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
        drawHalo(args);
    }
};
struct RatioDisplayWidget : BaseDisplayWidget {
    float* from = nullptr;
    float* to = nullptr;

    void drawLayer(const DrawArgs& args, int layer) override
    {
        if (layer != 1) { return; }

        std::shared_ptr<Font> font = APP->window->loadFont(
            asset::plugin(pluginInstance, "res/fonts/DSEG/DSEG7ClassicMini-Italic.ttf"));
        if (!font) { return; }

        nvgFontSize(args.vg, 11);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextLetterSpacing(args.vg, 1.0);

        // Text (from)
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);

        char fromString[10];
        snprintf(fromString, sizeof(fromString), "%2.0f", from ? *from : 1.0);

        Vec textPos = Vec(box.size.x / 2.0f - 3.0f, 16.0f);

        nvgFillColor(args.vg, lcdGhostColor);
        nvgText(args.vg, textPos.x, textPos.y, "88", NULL);
        nvgFillColor(args.vg, lcdTextColor);
        nvgText(args.vg, textPos.x, textPos.y, fromString, NULL);

        // Text (from)
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT);

        char toString[10];
        snprintf(toString, sizeof(toString), "%2.0f", to ? *to : 1.0);
        if (toString[0] == ' ') {
            toString[0] = toString[1];
            toString[1] = ' ';
        }

        textPos = Vec(box.size.x / 2.0f + 2.0f, 16.0f);

        nvgFillColor(args.vg, lcdGhostColor);
        nvgText(args.vg, textPos.x, textPos.y, "88", NULL);
        nvgFillColor(args.vg, lcdTextColor);
        nvgText(args.vg, textPos.x, textPos.y, toString, NULL);

        // Text (:)
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);

        textPos = Vec(box.size.x / 2.0f, 16.0f);

        nvgFillColor(args.vg, lcdTextColor);
        nvgText(args.vg, textPos.x, textPos.y, ":", NULL);

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        drawHalo(args);
    }
};
// NOLINTEND
}  // namespace comp