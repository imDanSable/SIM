#include "plugin.hpp"

Plugin* pluginInstance;  // NOLINT

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

    // Any other plugin initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when your module is created
    // to reduce startup times of Rack.
}
// static const std::vector<std::string> themes = {
//     "Dark",
//     "Light",
//     "Funky",
// };