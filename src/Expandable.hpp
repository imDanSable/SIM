#pragma once
#include <rack.hpp>
#include "Connectable.hpp"
#include "ModuleInstantiationMenu.hpp"
#include "ModuleX.hpp"
#include "constants.hpp"
#include "plugin.hpp"

// XXX Consider dropping the template part because I think we can do all
template <typename T>
class Expandable : public Connectable {
   public:
    Expandable(const ModelsListType& leftAllowedModels,
               const ModelsListType& rightAllowedModels,
               int leftLightId,
               int rightLightId)
        : Connectable(leftLightId, rightLightId, leftAllowedModels, rightAllowedModels),
          module(static_cast<T*>(this)){};
    void onExpanderChange(const engine::Module::ExpanderChangeEvent& e) override
    {
        checkLight(e.side, e.side ? rightExpander.module : leftExpander.module,
                   e.side ? rightAllowedModels : leftAllowedModels);
        e.side ? module->updateRightExpanders() : module->updateLeftExpanders();
    }

   protected:
    template <typename M, constants::sideType side>
    M* updateExpander(const ModelsListType& allowedModels)
    {
        Expander& expander =
            (side == constants::RIGHT ? module->rightExpander : module->leftExpander);
        Module* searchModule = module;
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

    T* module;  // NOLINT

   private:
    bool expanderUpdate = true;
    dsp::Timer expanderUpdateTimer;

    void onLeftChainChange(const ModuleX::ChainChangeEvent& /*e*/)
    {
        module->updateLeftExpanders();
    };
    void onRightChainChange(const ModuleX::ChainChangeEvent& /*e*/)
    {
        module->updateRightExpanders();
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

template <typename T>
MenuItem* createExpandableSubmenu(Expandable<T>* module, ModuleWidget* moduleWidget)
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