#pragma once
#include <cstddef>
#include <rack.hpp>
#include "Connectable.hpp"
#include "InX.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "ModuleIterator.hpp"
#include "ModuleX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "constants.hpp"
#include "plugin.hpp"
// XXX I believe when a second expander is connected with the callback pointing to this.
//  that the callback is not correctly set to nullptr.

/// @brief Base class for modules that can be expanded with expander modules derived from ModuleX.
/// @details Derive from this class if you want your module to be expandable.
/// Initialize the base class with the list of allowed models for the left and right side.
/// Have a member variable for each expander you want to use through its adapter.
/// Also pass the light ids for the connection lights for the left and right side.
///
// Example:
//
// class MyExpandable : public Expandable {
// public:
//   MyExpandable() : Expandable({modelModuleX1, modelModuleX2}, {modelModuleX3, modelModuleX4},
//   LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED)
// private:
//   MyInputAdapter myInputAdapter;
//   MyOutAdapter myOutAdapter;
// }
//
// Override updateLeftExpanders() and/or updateRightExpanders()
// and call adapter.setPtr(getExpander<ModuleX, sideType>({modelModuleX1, modelModuleX2}));
//
//    void updateLeftExpanders() override
//     {
//         myInputAdapter.setPtr(getExpander<MyInputExpander, LEFT>({models that don't consume your
//         expander, your expander model});
//     }
//
//     void updateRightExpanders() override
//     {
//         myOtherAdapter.setPtr(getExpander<MyOtherExpander, RIGHT>({models that don't consume you
//         as Expandable, your expander model});
//     }
//
// and in the destructor of your Expandable you should set all callbacks to nullptr
// e.g.: (we should fix this) and maybe we should define virtual ~Expandable = 0;???
// MyExpandable::~MyExpandable() override
// {
// if (rex) { rex->setChainChangeCallback(nullptr); }
// if (outx) { outx->setChainChangeCallback(nullptr); }
// if (inx) { inx->setChainChangeCallback(nullptr); }
// }

class Expandable : public Connectable {
   public:
    Expandable(const ModelsListType& leftAllowedModels,
               const ModelsListType& rightAllowedModels,
               int leftLightId,
               int rightLightId)
        : Connectable(leftLightId, rightLightId, leftAllowedModels, rightAllowedModels){};
    void onExpanderChange(const engine::Module::ExpanderChangeEvent& e) override
    {
        checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
                   e.side ? rightAllowedModels : leftAllowedModels);

        if (lastRightModule != rightExpander.module) {
            if (!rightExpander.module) {  // Implies e.side
                unsetExpanders(constants::RIGHT);
            }
            else {  // Implies rightExpander.module
            }
            lastRightModule = rightExpander.module;
        }
        if (lastLeftModule != leftExpander.module) {
            if (!leftExpander.module) {  // Implies !e.side
                unsetExpanders(constants::LEFT);
            }
            else {  // Implies leftExpander.module
            }
            lastLeftModule = leftExpander.module;
        }
        if (e.side) { updateRightExpanders(); }
        else {
            updateLeftExpanders();
        }
    }

    void onRemove() override
    {
        unsetExpanders(constants::RIGHT);
        unsetExpanders(constants::LEFT);
    }

    ModuleX* getExpander(Model* targetmodel,
                         const ModelsListType& allowedModels,
                         constants::sideType side,
                         Module* startModule)
    {
        for (Module* searchModule = startModule;;) {
            if (searchModule == nullptr) { return nullptr; }
            if (searchModule->model == targetmodel) {
                return dynamic_cast<ModuleX*>(searchModule);

                if (searchModule && std::find(allowedModels.begin(), allowedModels.end(),
                                              searchModule->model) == allowedModels.end()) {
                    return nullptr;
                };
            }
            searchModule = (side == constants::RIGHT ? searchModule->rightExpander.module
                                                     : searchModule->leftExpander.module);
        }
        assert(true);  // NOLINT
    }

   protected:
    virtual void updateLeftExpanders(){};
    virtual void updateRightExpanders(){};
    void unsetExpanders(constants::sideType side)
    {
        const ModelsListType& allowedModels =
            side == constants::RIGHT ? rightAllowedModels : leftAllowedModels;
        Module* startModule = side == constants::RIGHT ? lastRightModule : lastLeftModule;
        for (auto* model : allowedModels) {
            ModuleX* modulex = getExpander(model, allowedModels, side, startModule);
            if (modulex) {
                modulex->setChainChangeCallback(nullptr);
                DEBUG("Model: %s, expandable::onExpanderChange", model->name.c_str());
            }
        }
    };
    // XXX Create two untemplated versions of this function.
    // and make a cpp file

    template <typename TModuleX, constants::sideType side>
    TModuleX* getExpanderAndSetCallbacks2(Model* targetModel, const ModelsListType& allowedModels)
    {
        auto* targetModule =
            dynamic_cast<TModuleX*>(getExpander(targetModel, allowedModels, side, this));
        if (targetModule) {
            (side == constants::RIGHT) ? setRightChainChangeCallback(targetModule)
                                       : setLeftChainChangeCallback(targetModule);
        }
        return targetModule;
    }

    template <typename M, constants::sideType side>
    M* getExpanderAndSetCallbacks(const ModelsListType& allowedModels)
    {
        Expander& expander = (side == constants::RIGHT ? this->rightExpander : this->leftExpander);
        Module* searchModule = this;
        if (!expander.module) { return nullptr; }

        bool keepSearching = true;
        while (keepSearching) {
            searchModule = (side == constants::RIGHT ? searchModule->rightExpander.module
                                                     : searchModule->leftExpander.module);
            if (!searchModule) { break; }

            M* targetModule = dynamic_cast<M*>(searchModule);
            if (targetModule) {
                (side == constants::RIGHT) ? setRightChainChangeCallback(targetModule)
                                           : setLeftChainChangeCallback(targetModule);
                return targetModule;
            }
            keepSearching = searchModule && std::find(allowedModels.begin(), allowedModels.end(),
                                                      searchModule->model) != allowedModels.end();
        }
        return nullptr;
    }

   private:
    Module* lastLeftModule = nullptr;
    Module* lastRightModule = nullptr;

    void setLeftChainChangeCallback(ModuleX* expander)
    {
        expander->setChainChangeCallback(
            [this](const ModuleX::ChainChangeEvent& e) { this->updateLeftExpanders(); });
        // expander->setRightLightBrightness(1.F);
        // auto *other = dynamic_cast<ModuleX*>(expander->leftExpander.module);
        // if (other) { other->setRightLightBrightness(1.F);
        // }
    };

    void setRightChainChangeCallback(ModuleX* expander)
    {
        expander->setChainChangeCallback(
            [this](const ModuleX::ChainChangeEvent& e) { this->updateRightExpanders(); });
        // expander->setLeftLightBrightness(1.F);
        // auto* other = dynamic_cast<ModuleX*>(expander->rightExpander.module);
        // if (other) { other->setLeftLightBrightness(1.F); }
    };
};

static inline MenuItem* createExpandableSubmenu(Expandable* module, ModuleWidget* moduleWidget)
{
    return createSubmenuItem("Add Expander", "", [=](Menu* menu) {
        for (auto* compatible : module->leftAllowedModels) {
            auto* item = new ModuleInstantionMenuItem();  // NOLINT(cppcoreguidelines-owning-memory)
            item->text = "Add " + compatible->name;
            item->rightText = "←";
            item->module_widget = moduleWidget;
            item->right = false;
            item->model = compatible;
            menu->addChild(item);
        }
        for (auto* compatible : module->rightAllowedModels) {
            auto* item = new ModuleInstantionMenuItem();  // NOLINT(cppcoreguidelines-owning-memory)
            item->text = "Add " + compatible->name;
            item->rightText = "→";
            item->module_widget = moduleWidget;
            item->right = true;
            item->model = compatible;
            menu->addChild(item);
        }
    });
};