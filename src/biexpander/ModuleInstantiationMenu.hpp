#pragma once
#include <rack.hpp>
#include "../plugin.hpp"
// The license of the code below can be found in the file LICENSE_slime4rack.txt
// Thank you Coriander Pines!

class ModuleInstantionMenuItem : public rack::ui::MenuItem {
   public:
    bool right = true;             // NOLINT
    bool all = false;              // NOLINT
    int hp = 2;                    // NOLINT
    rack::plugin::Model* model{};  // NOLINT
    // rack::plugin::Model* leftModels{};  // NOLINT
    // rack::plugin::Model* rightModels{};  // NOLINT

    rack::app::ModuleWidget* module_widget{};  // NOLINT
    void onAction(const rack::event::Action& e) override
    {
        if (all) {
            // Execute all menu items in that are on the same level, except this one
            for (auto* item : parent->children) {
                // auto* menuItem = dynamic_cast<ModuleInstantionMenuItem*>(item);
                // DEBUG("item text = %s", menuItem->text.c_str());
                if (item != this) { item->onAction(rack::event::Action(e)); }
            }
            return;
        }
        float increment = right ? 2 * hp * rack::RACK_GRID_WIDTH : -hp * rack::RACK_GRID_WIDTH;
        float x = module_widget->box.pos.x + (right ? module_widget->box.size.x : 0);

        while (SIMWidget::simWidgetAtPostion(rack::math::Vec(x, module_widget->box.pos.y))) {
            x += increment;
        }
        DEBUG("x = %f", x);

        rack::math::Rect box = module_widget->box;
        rack::math::Vec pos = Vec(right ? x - box.getWidth() / 2 : x, box.pos.y);
        // rack::math::Vec pos = right ? box.pos.plus(rack::math::Vec(box.size.x, 0))
        //                             : box.pos.plus(rack::math::Vec(-hp * rack::RACK_GRID_WIDTH,
        //                             0));

        // Update ModuleInfo if possible
        rack::settings::ModuleInfo* mi =
            rack::settings::getModuleInfo(model->plugin->slug, model->slug);
        if (mi) {
            mi->added++;
            mi->lastAdded = rack::system::getUnixTime();
        }

        // The below flow comes from rack::app::browser::chooseModel

        // Create module
        rack::engine::Module* new_module = model->createModule();
        if (!new_module) {
            // DEBUG("Could not instantiate new module via menu item");
            return;
        }
        APP->engine->addModule(new_module);

        // Create module widget
        rack::app::ModuleWidget* new_widget = model->createModuleWidget(new_module);
        if (!new_widget) {
            // DEBUG("Could not instantiate new module widget via menu item");
            delete new_module;
            return;
        }
        APP->scene->rack->setModulePosNearest(new_widget, pos);
        APP->scene->rack->addModule(new_widget);

        // Load template preset
        new_widget->loadTemplate();

        // Record history manually; can't use a guard here!
        auto* h = new rack::history::ModuleAdd;
        h->name = "create expander module";
        h->setModule(new_widget);
        APP->history->push(h);
    }
};

// end LICENSE_slime4rack.txt