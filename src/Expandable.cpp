#include "plugin.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "Expandable.hpp"

// void createExpandableSubmenu(Expandable* module, ModuleWidget* moduleWidget, Menu* menu)
// {
//     for (auto compatible : module->leftAllowedModels)
//     {
//         auto item = new ModuleInstantionMenuItem();
//         item->text = "Add " + compatible->name;
//         item->rightText = "←";
//         item->module_widget = moduleWidget;
//         item->right = false;
//         item->model = compatible;
//         menu->addChild(item);
//     }
//     for (auto compatible : module->rightAllowedModels)
//     {
//         auto item = new ModuleInstantionMenuItem();
//         item->text = "Add " + compatible->name;
//         item->rightText = "→";
//         item->module_widget = moduleWidget;
//         item->right = true;
//         item->model = compatible;
//         menu->addChild(item);
//     }
// };