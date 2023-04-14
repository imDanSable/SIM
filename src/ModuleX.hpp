#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "constants.hpp"
#include "ModuleIterator.hpp"
using namespace rack;
using namespace std;
using namespace constants;


class ModuleX : public Module
{
public:
	typedef vector<Model *> ModelsListType;
	ModuleX(const ModelsListType& leftAllowedModels, 
		const ModelsListType& rightAllowedModels,
		std::function<void(float)> leftLightOn,
		std::function<void(float)> rightLightOn);
	void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override;
	void onRemove(const RemoveEvent &e) override;

	/// @brief Master modules should call process() in their own process() method.
	/// @brief this ensures that updateLeftCachedExpanderModules() and updateRightCachedExpanderModules() are called.
	void process(const ProcessArgs &args) override;

	template <typename T>
	void updateCachedExpanderModule(T *&modulePtr, sideType side, const ModelsListType &allowedModels)
	{
		modulePtr = nullptr;

		if (side == LEFT && leftExpander.module)
		{
			NoneOf search(allowedModels);
			ModuleReverseIterator found = std::find_if(++ModuleReverseIterator(this), ModuleReverseIterator(nullptr), search);
			modulePtr = dynamic_cast<T *>(&*found);
		}
		else if (side == RIGHT && rightExpander.module)
		{
			NoneOf search(allowedModels);
			ModuleIterator found = std::find_if(++ModuleIterator(this), ModuleIterator(nullptr), search);
			modulePtr = dynamic_cast<T *>(&*found);
		}
	}

	 virtual void updateRightCachedExpanderModules() {};
	 virtual void updateLeftCachedExpanderModules()  {};

	friend class ModuleIterator;
	friend class ModuleReverseIterator;

private:
	bool expanderUpdate = true;
	dsp::Timer expanderUpdateTimer;
	const ModelsListType leftAllowedModels = ModelsListType{modelReX, modelInX};
	const ModelsListType rightAllowedModels = ModelsListType{modelReX, modelSpike};

	/// @brief Cached, so we can handle connection lights on behalf of ascendants
	const function<void(float)> leftLightOn, rightLightOn;

	template <typename Derived>
	const ModelsListType &getConsumesModules(sideType side) const
	{
		return (side == LEFT) ? static_cast<const Derived *>(this)->consumesLeftward : static_cast<const Derived *>(this)->consumesRightward;
	}
	/// @brief What consumes this module going left and right
	ModelsListType consumesLeftward, consumesRightward;
};

// The license of the code below can be found in the file LICENSE_slime4rack.txt
// Thank you Coriander Pines!

class ModuleInstantionMenuItem : public rack::ui::MenuItem
{
public:
	bool right = true;
	int hp = 2;
	rack::app::ModuleWidget *module_widget;
	rack::plugin::Model *model;

	ModuleInstantionMenuItem() { rightText = "!!"; }

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
			WARN("Could not instantiate new module via menu item");
			return;
		}
		APP->engine->addModule(new_module);

		// Create module widget
		rack::app::ModuleWidget *new_widget = model->createModuleWidget(new_module);
		if (!new_widget)
		{
			WARN("Could not instantiate new module widget via menu item");
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