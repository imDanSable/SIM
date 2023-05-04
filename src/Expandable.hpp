#pragma once
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
// e.g.:
// XXX This is not correct. And probably not needed anymore.
// MyExpandable::~MyExpandable() override
// {
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
        // XXX Fix this mess
        // extract or

        // for (auto* model : leftAllowedModels) {
        //     ModuleX* modulex = getExpander(model, leftAllowedModels, constants::LEFT, this);
        //     if (modulex) { modulex->setChainChangeCallback(nullptr); }
        // }
        // Work out a signal slot system so we don't have to search and traverse the list over and
        // over Keep record.. Send a nullptr.

        checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
                   e.side ? rightAllowedModels : leftAllowedModels);

        if (lastRightExpander != rightExpander.module) {
            if (!rightExpander.module) {  // Implies e.side
                if (!rightExpander.module) {
                    for (auto* model : rightAllowedModels) {
                        ModuleX* modulex = getExpander(model, rightAllowedModels, constants::RIGHT,
                                                       lastRightExpander);
                        if (modulex) { modulex->setChainChangeCallback(nullptr); }
                    }
                }
                else {  // Implies rightExpander.module
                    updateRightExpanders();
                }
            }
            lastRightExpander = rightExpander.module;
        }
        if (lastLeftExpander != leftExpander.module) {
            if (!leftExpander.module) {  // Implies !e.side
                if (!leftExpander.module) {
                    for (auto* model : leftAllowedModels) {
                        ModuleX* modulex = getExpander(model, leftAllowedModels, constants::LEFT,
                                                       lastLeftExpander);
                        if (modulex) { modulex->setChainChangeCallback(nullptr); }
                    }
                }
            }
            else {  // Implies leftExpander.module
                updateLeftExpanders();
            }
        }

        // e.side&& left ? updateRightExpanders() : updateLeftExpanders();
        DEBUG("Expandable::onExpanderChange");
    }

    void onRemove() override
    {
        // XXX Using getExpander() and setChainChangeCallback() is expensive
        //  A lot of double searching is being done here.
        for (auto* model : leftAllowedModels) {
            ModuleX* modulex = getExpander(model, leftAllowedModels, constants::LEFT, this);
            if (modulex) { modulex->setChainChangeCallback(nullptr); }
        }
        for (auto* model : rightAllowedModels) {
            ModuleX* modulex = getExpander(model, rightAllowedModels, constants::RIGHT, this);
            if (modulex) { modulex->setChainChangeCallback(nullptr); }
        }
        DEBUG("Expandable::onRemove");
    }
    virtual void updateLeftExpanders(){};
    virtual void updateRightExpanders(){};

    ModuleX* getExpander(Model* targetmodel,
                         const ModelsListType& allowedModels,
                         constants::sideType side,
                         Module* startModule)
    {
        Module* searchModule = startModule;
        Expander& expander =
            (side == constants::RIGHT ? searchModule->rightExpander : searchModule->leftExpander);
        if (!expander.module) { return nullptr; }

        bool keepSearching = true;
        while (keepSearching) {
            searchModule = (side == constants::RIGHT ? searchModule->rightExpander.module
                                                     : searchModule->leftExpander.module);
            if (!searchModule) { break; }

            if (targetmodel == searchModule->model) {
                return dynamic_cast<ModuleX*>(searchModule);
                // auto* targetModule = dynamic_cast<ModuleX*>(searchModule);
                // (side == constants::RIGHT) ? setRightChainChangeCallback(targetModule)
                //                            : setLeftChainChangeCallback(targetModule);
                // return targetModule;
            }
            keepSearching = searchModule && std::find(allowedModels.begin(), allowedModels.end(),
                                                      searchModule->model) != allowedModels.end();
        }
        return nullptr;
    }

   protected:
    // XXX Create two untemplated versions of this function.
    // and make a cpp file
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
    Module* lastLeftExpander = nullptr;
    Module* lastRightExpander = nullptr;

    void onLeftChainChange(const ModuleX::ChainChangeEvent& /*e*/)
    {
        updateLeftExpanders();
    };
    void onRightChainChange(const ModuleX::ChainChangeEvent& /*e*/)
    {
        updateRightExpanders();
    };

    void setLeftChainChangeCallback(ModuleX* expander)
    {
        expander->setChainChangeCallback(
            [this](const ModuleX::ChainChangeEvent& e) { this->onLeftChainChange(e); });
        // expander->setRightLightBrightness(1.F);
        // auto *other = dynamic_cast<ModuleX*>(expander->leftExpander.module);
        // if (other) { other->setRightLightBrightness(1.F);
        // }
    };

    void setRightChainChangeCallback(ModuleX* expander)
    {
        expander->setChainChangeCallback(
            [this](const ModuleX::ChainChangeEvent& e) { this->onRightChainChange(e); });
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