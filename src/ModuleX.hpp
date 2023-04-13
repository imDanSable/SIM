#pragma once
#include <rack.hpp>
#include <map>
#include "plugin.hpp"
#include "set"
#include "constants.hpp"
using namespace rack;
using namespace std;
using namespace constants;

class ModuleX : public Module
{
public:
	ModuleX();
	void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override;
	void onRemove(const RemoveEvent &e) override;

	Module *getModel(const Model *model, const sideType side);
	ModuleX *addAllowedModel(Model *model, sideType side);
	ModuleX *setLeftLightOn(std::function<void(float)> lightOn);
	ModuleX *setRightLightOn(std::function<void(float)> lightOn);

	class ModelPredicate
	{
	public:
		explicit ModelPredicate(Model *targetModel) : targetModel_(targetModel) {}

		bool operator()(const Module &module) const
		{
			return module.model == targetModel_;
		}

	private:
		Model *targetModel_;
	};
	class MultiModelPredicate
	{
	public:
		explicit MultiModelPredicate(const std::vector<Model *> &targetModels) : targetModels_(targetModels) {}

		bool operator()(const Module &module) const
		{
			return std::find(targetModels_.begin(), targetModels_.end(), module.model) != targetModels_.end();
			// return targetModels_.find(module.model) != targetModels_.end();
		}
		private:
		std::vector<Model *> targetModels_;
	};
	class ModuleIterator
	{
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = Module;
		using difference_type = std::ptrdiff_t;
		using pointer = Module *;
		using reference = Module &;

		ModuleIterator(pointer ptr) : ptr_(ptr) {}

		// Dereference operator
		reference operator*() const { return *static_cast<pointer>(ptr_); }
		pointer operator->() { return static_cast<pointer>(ptr_); }

		// Pre-increment operator
		ModuleIterator &operator++()
		{
			if (ptr_)
				ptr_ = static_cast<pointer>(ptr_->rightExpander.module);
			return *this;
		}

		// Pre-decrement operator
		ModuleIterator &operator--()
		{
			if (ptr_)
				ptr_ = static_cast<pointer>(ptr_->leftExpander.module);
			return *this;
		}

		// Post-increment operator
		ModuleIterator operator++(int)
		{
			ModuleIterator tmp(*this);
			++(*this);
			return tmp;
		}

		// Post-decrement operator
		ModuleIterator operator--(int)
		{
			ModuleIterator tmp(*this);
			--(*this);
			return tmp;
		}

		// Equality operator
		bool operator==(const ModuleIterator &other) const { return ptr_ == other.ptr_; }

		// Inequality operator
		bool operator!=(const ModuleIterator &other) const { return ptr_ != other.ptr_; }

	private:
		pointer ptr_;
	};
	class ModuleReverseIterator
	{
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = Module;
		using difference_type = std::ptrdiff_t;
		using pointer = Module *;
		using reference = Module &;

		ModuleReverseIterator(Module *ptr) : ptr_(static_cast<pointer>(ptr)) {}

		// Dereference operator
		reference operator*() const { return *ptr_; }
		pointer operator->() { return ptr_; }

		// Pre-increment operator
		ModuleReverseIterator &operator++()
		{
			if (ptr_)
				ptr_ = static_cast<pointer>(ptr_->leftExpander.module);
			return *this;
		}

		// Pre-decrement operator
		ModuleReverseIterator &operator--()
		{
			if (ptr_)
				ptr_ = static_cast<pointer>(ptr_->rightExpander.module);
			return *this;
		}

		// Post-increment operator
		ModuleReverseIterator operator++(int)
		{
			ModuleReverseIterator tmp(*this);
			++(*this);
			return tmp;
		}

		// Post-decrement operator
		ModuleReverseIterator operator--(int)
		{
			ModuleReverseIterator tmp(*this);
			--(*this);
			return tmp;
		}

		// Equality operator
		bool operator==(const ModuleReverseIterator &other) const { return ptr_ == other.ptr_; }

		// Inequality operator
		bool operator!=(const ModuleReverseIterator &other) const { return ptr_ != other.ptr_; }

	private:
		pointer ptr_;
	};

	// class Iterator
	// {
	// public:
	// 	Iterator(ModuleX *module) : current(module) {}

	// 	// Prefix increment operator
	// 	Iterator &operator++()
	// 	{
	// 		current = getNextConnected(RIGHT);
	// 		return *this;
	// 	}

	// 	// Prefix decrement operator
	// 	Iterator &operator--()
	// 	{
	// 		current = getNextConnected(LEFT);
	// 		return *this;
	// 	}
	// 	// Dereference operator
	// 	ModuleX &operator*() const
	// 	{
	// 		return *current;
	// 	}

	// 	// Equality comparison operator
	// 	bool operator==(const Iterator &other) const
	// 	{
	// 		return current == other.current;
	// 	}

	// 	// Inequality comparison operator
	// 	bool operator!=(const Iterator &other) const
	// 	{
	// 		return !(*this == other);
	// 	}

	// 	// Return an iterator to the end of the list//
	// 	static Iterator end()
	// 	{
	// 		return Iterator(nullptr);
	// 	}

	// private:
	// 	ModuleX *getNextConnected(const sideType side)
	// 	{
	// 		Expander &expander = (side == LEFT) ? current->leftExpander : current->rightExpander;
	// 		std::vector<Model *> &models = (side == LEFT) ? current->leftModels : current->rightModels;
	// 		if (expander.module == NULL)
	// 		{
	// 			return NULL;
	// 		}
	// 		for (Model *model : models)
	// 		{
	// 			if (model == expander.module->model)
	// 			{
	// 				return static_cast<ModuleX *>(expander.module);
	// 			}
	// 		}
	// 		return nullptr;
	// 	}
	// 	ModuleX *current;
	// };

public:
	ModuleX* masterModule = nullptr;
	sideType masterSide = LEFT;
	void invalidateCachedModule(const Module* module);
	void updateTraversalCache(const sideType side);

private:
	/// @brief <<ThisStopsSearching, ForThat> on the left
	const std::multimap<const Model *, const Model *> leftConsumeTable = {
		{modelInX, modelReX},
		{modelSpike, modelReX}};
	/// @brief <<ThisStopsSearching, ForThat> on the right
	const std::multimap<const Model *, const Model *> rightConsumeTable{
		{modelReX, modelOutX}};

	// ModuleX *getNextConnected(const sideType side);

	Module *lastLeftModule = nullptr;
	Module *lastRightModule = nullptr;

	typedef vector<Model *> ModelsListType;
	typedef std::vector<Module *> ModuleTraverselCachType;
	ModuleTraverselCachType leftTraversalCache, rightTraversalCache;
	void structureChanged(sideType side);

	std::vector<Module *> connectedModules;

	/// @brief Cached, so we can handle connection lights on behalf of ascendants
	function<void(float)> leftLightOn, rightLightOn;

	/// @brief What this module accepts on left and right side
	// set<Model *> leftModels, rightModels;
	ModelsListType leftModels, rightModels;
};

// class ModuleIter {
// public:
// 	ModuleIter(ModuleX* module) : current(module) {}

//     // Prefix increment operator
//     ModuleIter& operator++() {
// 		ModuleX *rExp = current->getNextConnected(RIGHT);
//         if (current && current->rightExpander.module) {
//             current = current->rightExpander.module;
//         }
//         return *this;
//     }

//     // Prefix decrement operator
//     ListIterator& operator--() {
//         if (current) {
//             current = current->prev;
//         }
//         return *this;
//     }
// private:
// 	ModuleX* current;

// }

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