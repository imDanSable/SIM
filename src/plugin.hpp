#pragma once
#include <rack.hpp>

// #undef DEBUG
// #define DEBUG(format, ...) ((void)0)  // nop

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
// extern Model* modelDebugX;   // NOLINT

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

    inline std::string getComponentPath() const
    {
        return "res/components/" + getCurrentThemeName() + "/";
    }
    static inline std::string getPanelPath(const std::string& modelName,
                                           const std::string& themeName = "vapor")
    {
        return "res/panels/" + themeName + "/" + modelName + ".svg";
    }
    int getDefaultTheme() const;
    int getDefaultDarkTheme() const;
    void setDefaultTheme(int theme);
    void setDefaultDarkTheme(int theme);

    void readDefaultTheme();
    void readDefaultDarkTheme();
    std::string getCurrentThemeName() const
    {
        return themes[getCurrentTheme()];
    }
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
    // static list of all sim widgets
    static std::vector<SIMWidget*> simWidgets;

   public:
    static bool simWidgetAtPostion(math::Vec pos)
    {
        // No ranges on mac
        // return std::ranges::any_of(simWidgets,
        //                            [&](auto widget) { return widget->box.contains(pos); });
        bool foundWidget = false;
        for (auto* widget : simWidgets) {
            if (widget->box.contains(pos)) {
                foundWidget = true;
                break;
            }
        }
        return foundWidget;
    }
    SIMWidget()
    {
        // Add yourself to a static list of SIMWidgets for easier search
        simWidgets.push_back(this);
        DEBUG("SIMWidget created");
    }
    ~SIMWidget() override
    {
        // Remove yourself from the static list of SIMWidgets
        simWidgets.erase(std::remove(simWidgets.begin(), simWidgets.end(), this), simWidgets.end());
    }
    void setSIMPanel(const std::string& modelName)
    {
        setPanel(createPanel(
            asset::plugin(pluginInstance, Themable::getPanelPath(
                                              modelName, themes[themeInstance->getDefaultTheme()])),
            asset::plugin(
                pluginInstance,
                Themable::getPanelPath(modelName, themes[themeInstance->getDefaultDarkTheme()]))));
    }
    void appendContextMenu(Menu* menu) override
    {
        menu->addChild(new MenuSeparator);
        menu->addChild(createIndexSubmenuItem(
            "Default (non Dark) Theme", themes, [&]() { return themeInstance->getDefaultTheme(); },
            [&](int theme) { themeInstance->setDefaultTheme(theme); }));
        menu->addChild(createIndexSubmenuItem(
            "Default Dark Theme", themes, [&]() { return themeInstance->getDefaultDarkTheme(); },
            [&](int theme) { themeInstance->setDefaultDarkTheme(theme); }));
    };
    void step() override
    {
        if (module) {
            int currentTheme = themeInstance->getCurrentTheme();

            if (lastTheme != currentTheme) {
                lastTheme = currentTheme;
                setSIMPanel(module->model->name);
            }
        }
        Widget::step();
    }

   private:
    int lastTheme = -1;
    Themable* themeInstance = &Themable::getInstance();
};