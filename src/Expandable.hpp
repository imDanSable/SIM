#pragma once
#include <mpark/variant.hpp>
#include <rack.hpp>
#include "Connectable.hpp"
#include "InX.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "ModuleX.hpp"
#include "OutX.hpp"
#include "Rex.hpp"
#include "constants.hpp"
#include "plugin.hpp"

template <typename... Ts>
using Variant = mpark::variant<Ts...>;
// XXX Consider dropping the template part because I think we can do all

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
    std::vector<Variant<RexAdapter, InxAdapter, OutxAdapter>> adapters;
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

            // ModuleX *modulex = dynamic_cast<ModuleX *>(searchModule);
            M* targetModule = dynamic_cast<M*>(searchModule);
            if (targetModule) {
                (side == constants::RIGHT) ? setRightChainChangeCallback(targetModule)
                                           : setLeftChainChangeCallback(targetModule);
                // DEBUG("Setting callback for %s", searchModule->model->name.c_str());
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