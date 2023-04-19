#pragma once
#include "plugin.hpp"
// The license of the code below can be found in the file LICENSE_slime4rack.txt
// Thank you Coriander Pines!

class ModuleInstantionMenuItem : public rack::ui::MenuItem
{
public:
	bool right = true;
	int hp = 2;
	rack::app::ModuleWidget *module_widget;
	rack::plugin::Model *model;


	void onAction(const rack::event::Action &e) override
	{
		rack::math::Rect box = module_widget->box;
		rack::math::Vec pos = right ? box.pos.plus(rack::math::Vec(box.size.x, 0)) : box.pos.plus(rack::math::Vec(-hp * RACK_GRID_WIDTH, 0));

		// Update ModuleInfo if possible
		rack::settings::ModuleInfo *mi = rack::settings::getModuleInfo(model->plugin->slug, model->slug);
		if (mi)
		{
			mi->added++;
			mi->lastAdded = rack::system::getUnixTime();
		}

		// The below flow comes from rack::app::browser::chooseModel

		// Create module
		rack::engine::Module *new_module = model->createModule();
		if (!new_module)
		{
			//DEBUG("Could not instantiate new module via menu item");
			return;
		}
		APP->engine->addModule(new_module);

		// Create module widget
		rack::app::ModuleWidget *new_widget = model->createModuleWidget(new_module);
		if (!new_widget)
		{
			//DEBUG("Could not instantiate new module widget via menu item");
			delete new_module;
			return;
		}
		APP->scene->rack->setModulePosNearest(new_widget, pos);
		APP->scene->rack->addModule(new_widget);

		// Load template preset
		new_widget->loadTemplate();

		// Record history manually; can't use a guard here!
		rack::history::ModuleAdd *h = new rack::history::ModuleAdd;
		h->name = "create expander module";
		h->setModule(new_widget);
		APP->history->push(h);
	}
};

// end LICENSE_slime4rack.txt