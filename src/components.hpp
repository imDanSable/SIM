#pragma once
#include <string>
#include "plugin.hpp"
// #include "nanovg.h"
// #include "helpers.hpp"
// #include "app/ModuleWidget.hpp"

struct SmallPort : app::SvgPort {
  SmallPort() {
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SmallPort.svg")));
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


struct RSLabelCentered : LedDisplay
{
    std::shared_ptr<Font> font;
    int fontSize;
    std::string text;
    NVGcolor color;

    RSLabelCentered(Vec mm, const char *str, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff)) : RSLabelCentered(mm.x, mm.y, str, fontSize, colour)
    {
    }
    RSLabelCentered(int x, int y, const char *str = "", int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff))
    {
        // font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Orbitron-Black.ttf"));
        // font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/BlasterEternalItalic-m2W5.ttf"));
        font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Roboto-Medium.ttf"));
        this->fontSize = fontSize;
        box.pos = Vec(x, y);
        text = str;
        color = colour;
    }

    void draw(const DrawArgs &args) override
    {
        if (font->handle >= 0)
        {
            bndSetFont(font->handle);

            nvgFontSize(args.vg, fontSize);
            nvgFontFaceId(args.vg, font->handle);
            nvgTextLetterSpacing(args.vg, 0);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER);

            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, color);
            nvgText(args.vg, 0, 0, text.c_str(), NULL);
            nvgStroke(args.vg);

            bndSetFont(APP->window->uiFont->handle);
        }
    }
};

struct RSLabelCentered45 : RSLabelCentered
{
    RSLabelCentered45(Vec mm, const char *str, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff)) : RSLabelCentered45(mm.x, mm.y, str, fontSize, colour)
    {
    }
    RSLabelCentered45(int x, int y, const char *str = "", int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff)) : RSLabelCentered(x, y, str, fontSize, colour)
    {
    }

    void draw(const DrawArgs &args) override
    {
        if (font->handle >= 0)
        {
            bndSetFont(font->handle);

            nvgFontSize(args.vg, fontSize);
            nvgFontFaceId(args.vg, font->handle);
            nvgTextLetterSpacing(args.vg, 0);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
            nvgRotate(args.vg, -M_PI_4);

            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, color);
            nvgText(args.vg, 0, 0, text.c_str(), NULL);
            nvgStroke(args.vg);

            bndSetFont(APP->window->uiFont->handle);
        }
    }
};
struct CenteredLabeledJack : Widget
{
    CenteredLabeledJack(float centre, float y, const char *str, ModuleWidget *moduleWidget, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff))
    : Widget(*moduleWidget)
    // : ModuleWidget(moduleWidget)
    {
        addChild(new RSLabelCentered(mm2px(centre), mm2px(y - 4.3), str, fontSize, colour));
    }
    // {
    //     addChild(new RSLabelCentered(mm2px(centre), mm2px(y - 4.3), str, fontSize, colour));
    // }
};
struct CenteredLabeledInputJack : Widget
{
    CenteredLabeledInputJack(float centre, float y, const char *str, ModuleWidget *moduleWidget, int id, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff)) 
    // : CenteredLabeledJack(mm2px(centre), mm2px(y), str, moduleWidget, fontSize, colour)
    {
        addChild(new RSLabelCentered(mm2px(centre), mm2px(y - 4.3), str, fontSize, colour));
        moduleWidget->addInput(createInputCentered<SmallPort>(mm2px(Vec(centre, y)), moduleWidget->getModule(), id));
    }
};
struct CenteredLabeledOutputJack : Widget
{
    CenteredLabeledOutputJack(float centre, float y, const char *str, ModuleWidget *moduleWidget, int id, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff))
    // : CenteredLabeledJack(mm2px(centre), mm2px(y), str, moduleWidget, fontSize, colour)
    {
        addChild(new RSLabelCentered(mm2px(centre), mm2px(y - 4.3), str, fontSize, colour));
		moduleWidget->addOutput(createOutputCentered<SmallPort>(mm2px(Vec(centre, y)), moduleWidget->getModule(), id));
    }
};

struct CenteredLabeledSIMKnob : Widget
{
    CenteredLabeledSIMKnob(float centre, float y, const char *str, ModuleWidget *moduleWidget, int id, int fontSize = 10, const NVGcolor &colour = nvgRGB(0xff, 0xff, 0xff))
    // : CenteredLabeledJack(mm2px(centre), mm2px(y), str, moduleWidget, fontSize, colour)
    {
        addChild(new RSLabelCentered(mm2px(centre), mm2px(y - 6.3), str, fontSize, colour));
		// moduleWidget->addOutput(createOutputCentered<SmallPort>(mm2px(Vec(centre, y)), moduleWidget->getModule(), id));
		// moduleWidget->addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(centre, 100.0f)),moduleWidget->getModule(),id));
		moduleWidget->addParam(createParamCentered<SIMKnob>(mm2px(Vec(centre, 100.0f)),moduleWidget->getModule(),id));
    }
};