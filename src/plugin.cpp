#include "plugin.hpp"
Plugin* pluginInstance;

static const std::string settingsFileName = asset::user("SIM.json");

int defaultTheme = 0;
int defaultDarkTheme = 1;

void Themable::setDefaultTheme(int theme)
{
    if (defaultTheme != theme) {
        FILE* file = fopen(settingsFileName.c_str(), "w");
        if (file) {
            defaultTheme = theme;
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "defaultTheme", json_integer(theme));
            json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
            fclose(file);
            json_decref(rootJ);
        }
    }
}

void Themable::readDefaultTheme()
{
    FILE* file = fopen(settingsFileName.c_str(), "r");
    if (file) {
        json_error_t error;
        json_t* rootJ = json_loadf(file, 0, &error);
        json_t* jsonVal = json_object_get(rootJ, "defaultTheme");
        if (jsonVal) defaultTheme = json_integer_value(jsonVal);
        fclose(file);
        json_decref(rootJ);
    }
}

void Themable::setDefaultDarkTheme(int theme)
{
    if (defaultDarkTheme != theme) {
        FILE* file = fopen(settingsFileName.c_str(), "w");
        if (file) {
            defaultDarkTheme = theme;
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "defaultDarkTheme", json_integer(theme));
            json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
            fclose(file);
            json_decref(rootJ);
        }
    }
}

void Themable::readDefaultDarkTheme()
{
    FILE* file = fopen(settingsFileName.c_str(), "r");
    if (file) {
        json_error_t error;
        json_t* rootJ = json_loadf(file, 0, &error);
        json_t* jsonVal = json_object_get(rootJ, "defaultDarkTheme");
        if (jsonVal) defaultDarkTheme = json_integer_value(jsonVal);
        fclose(file);
        json_decref(rootJ);
    }
}

int Themable::getDefaultTheme() const
{
    return defaultTheme;
}

int Themable::getDefaultDarkTheme() const
{
    return defaultDarkTheme;
}
void init(Plugin* p)
{
    pluginInstance = p;

    // Add modules here
    p->addModel(modelBlank);
    p->addModel(modelPhi);
    p->addModel(modelArr);
    p->addModel(modelVia);
    p->addModel(modelCoerce);
    p->addModel(modelCoerce6);
    p->addModel(modelSpike);
    p->addModel(modelReX);
    p->addModel(modelInX);
    p->addModel(modelOutX);
    p->addModel(modelTie);
    p->addModel(modelBank);
    p->addModel(modelModX);
    p->addModel(modelOpX);
    p->addModel(modelAlgoX);
    p->addModel(modelGaitX);
    p->addModel(modelDebugX);

    Themable::getInstance();
    // Any other plugin initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when your module is created
    // to reduce startup times of Rack.
}
