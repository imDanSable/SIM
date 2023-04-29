#pragma once
#include <rack.hpp>
#include "Connectable.hpp"
#include "InX.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "ModuleX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "constants.hpp"
#include "plugin.hpp"

/// @brief Base class for modules that can be expanded with expander modules derived from ModuleX.
/// @details Derive from this class if you want your module to be expandable.
// All access to the expander should be done through adapters.
// For each compatible expander you should have a corresponding adapter member
// e.g.:
//
// class MyExpandable : public Expandable {
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
//
// MyExpandable::~MyExpandable() override
// {
//     if (myInputAdapter) { myInputAdapter.setPtr(nullptr); }
//     if (myOutAdapter) { myOutAdapter.setPtr(nullptr); }
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
        e.side ? updateRightExpanders() : updateLeftExpanders();
    }
    virtual void updateLeftExpanders(){};
    virtual void updateRightExpanders(){};

   protected:
    // XXX Create two untemplated versions of this function.
    // and make a cpp file
    template <typename M, constants::sideType side>
    M* getExpander(const ModelsListType& allowedModels)
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
    };

    void setRightChainChangeCallback(ModuleX* expander)
    {
        expander->setChainChangeCallback(
            [this](const ModuleX::ChainChangeEvent& e) { this->onRightChainChange(e); });
    };
};

inline MenuItem* createExpandableSubmenu(Expandable* module, ModuleWidget* moduleWidget)
{
    return createSubmenuItem("Add Expander", "", [=](Menu* menu) {
        for (auto compatible : module->leftAllowedModels) {
            auto* item = new ModuleInstantionMenuItem();  // NOLINT(cppcoreguidelines-owning-memory)
            item->text = "Add " + compatible->name;
            item->rightText = "←";
            item->module_widget = moduleWidget;
            item->right = false;
            item->model = compatible;
            menu->addChild(item);
        }
        for (auto compatible : module->rightAllowedModels) {
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