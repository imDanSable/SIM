#pragma once
#include <rack.hpp>

using namespace rack;  // NOLINT

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;  // NOLINT

// Declare each Model, defined in each module source file
extern Model* modelBlank;    // NOLINT
extern Model* modelPhi;      // NOLINT
extern Model* modelArr;      // NOLINT
extern Model* modelVia;      // NOLINT
extern Model* modelCoerce;   // NOLINT
extern Model* modelCoerce6;  // NOLINT
extern Model* modelSpike;    // NOLINT
extern Model* modelReX;      // NOLINT
extern Model* modelInX;      // NOLINT
extern Model* modelOutX;     // NOLINT
extern Model* modelTie;      // NOLINT
extern Model* modelBank;     // NOLINT
extern Model* modelModX;     // NOLINT
extern Model* modelOpX;      // NOLINT
extern Model* modelAlgoX;    // NOLINT
extern Model* modelGaitX;    // NOLINT
extern Model* modelDebugX;   // NOLINT

inline std::string getSvgPath(const std::string& modelName, const std::string& themeName = "vapor")
{
    return "res/panels/" + themeName + "/" + modelName + ".svg";
}
static const std::vector<std::string> themes = {
    "vapor",
    "dark",
    "light",
};

struct Themable {
   public:
    Themable(Themable const&) = delete;
    void operator=(Themable const&) = delete;
    static Themable& getInstance()
    {
        static Themable instance;
        return instance;
    }
    int getDefaultTheme() const;
    int getDefaultDarkTheme() const;
    void setDefaultTheme(int theme);
    void setDefaultDarkTheme(int theme);

    void readDefaultTheme();
    void readDefaultDarkTheme();
    int getCurrentTheme() const
    {
        return settings::preferDarkPanels ? defaultDarkTheme : defaultTheme;
    }

   private:
    Themable()
    {
        readDefaultTheme();
        readDefaultDarkTheme();
    }
    int currentTheme = 0;
    int defaultTheme = getDefaultTheme();
    int defaultDarkTheme = getDefaultDarkTheme();
    int prevTheme = -1;
    int prevDarkTheme = -1;
    bool drawn = false;
};

class SIMWidget : public ModuleWidget {
   public:
    void setSIMPanel(const std::string& modelName)
    {
        Themable& theme = Themable::getInstance();
        setPanel(createPanel(
            asset::plugin(pluginInstance, getSvgPath(modelName, themes[theme.getDefaultTheme()])),
            asset::plugin(pluginInstance,
                          getSvgPath(modelName, themes[theme.getDefaultDarkTheme()]))));
    }
    void appendContextMenu(Menu* menu) override
    {
        Themable& themeInstance = Themable::getInstance();
        menu->addChild(new MenuSeparator);
        menu->addChild(createIndexSubmenuItem(
            "Default (non Dark) Theme", themes, [&]() { return themeInstance.getDefaultTheme(); },
            [&](int theme) { themeInstance.setDefaultTheme(theme); }));
        menu->addChild(createIndexSubmenuItem(
            "Default Dark Theme", themes, [&]() { return themeInstance.getDefaultDarkTheme(); },
            [&](int theme) { themeInstance.setDefaultDarkTheme(theme); }));
    };
    void step() override
    {
        if (module) {
            int currentTheme = themeInstance.getCurrentTheme();

            if (lastTheme != currentTheme) {
                lastTheme = currentTheme;
                setSIMPanel(module->model->name);
            }
        }
        Widget::step();
    }
    // int panelTheme =
    //     isDark(module ? (&((static_cast<BlackHoles*>(module))->panelTheme)) : NULL) ? 1 :
    //     0;
    // if (lastPanelTheme != panelTheme) {
    //     lastPanelTheme = panelTheme; //     SvgPanel* panel = static_cast<SvgPanel*>(getPanel());
    //     panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
    //     panel->fb->dirty = true;
    // }
    //     Widget::step();
    // }
    // void step() override
    // {
    //     if (module) {
    //         // if (module->defaultTheme != getDefaultTheme()) {
    //         //     module->defaultTheme = getDefaultTheme();
    //         //     if (module->currentTheme == 0) module->prevTheme = -1;
    //         // }
    //         // if (module->defaultDarkTheme != getDefaultDarkTheme()) {
    //         //     module->defaultDarkTheme = getDefaultDarkTheme();
    //         //     if (module->currentTheme == 0) module->prevTheme = -1;
    //         // }
    //         // if (module->prevTheme != module->currentTheme) {
    //         //     module->prevTheme = module->currentTheme;
    //         //     setPanel(createPanel(
    //         //         asset::plugin(pluginInstance,
    //         //                       faceplatePath(moduleName, module->currentThemeStr())),
    //         //         asset::plugin(pluginInstance,
    //         //                       faceplatePath(moduleName,
    //         module->currentThemeStr(true)))));
    //         // }
    //     }
    //     Widget::step();
    // }

   private:
    int lastTheme = -1;
    std::string modelName;
    Themable& themeInstance = Themable::getInstance();
};