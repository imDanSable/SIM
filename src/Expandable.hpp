#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "Connectable.hpp"
#include "constants.hpp"

template <typename T>
class Expandable : public Module, protected Connectable
{
public:
    Expandable(const ModelsListType &leftAllowedModels, const ModelsListType &rightAllowedModels,
               std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn)
        : Connectable(leftLightOn, rightLightOn, leftAllowedModels, rightAllowedModels),
          module(static_cast<T *>(this)){};
    void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override
    {
        //DEBUG("Model: %s, Expandable::onExpanderChange(%s)", model->name.c_str(), e.side ? "right" : "left");
        // checkLight(e.side, module, e.side ? rightAllowedModels : leftAllowedModels);
        checkLight(e.side, e.side ? rightExpander.module : leftExpander.module, e.side ? rightAllowedModels : leftAllowedModels);
        e.side ? module->updateRightExpanders() : module->updateLeftExpanders();
    }

protected:
    template <typename M, sideType side>
    M *updateExpander(const ModelsListType &allowedModels)
    {
        //DEBUG("Model: %s, Expandable::updateExpander(%s), Pointer type: %s", model->name.c_str(), side == RIGHT ? "right" : "left", typeid(M).name());
        Expander &expander = (side == RIGHT ? module->rightExpander : module->leftExpander);
        Module *searchModule = module;
        if (!expander.module)
            return nullptr;

        bool keepSearching = true;
        while (keepSearching)
        {
            searchModule = (side == RIGHT ? searchModule->rightExpander.module : searchModule->leftExpander.module);
            if (!searchModule)
                break;

            // ModuleX *modulex = dynamic_cast<ModuleX *>(searchModule);
            M *targetModule = dynamic_cast<M *>(searchModule);
            if (targetModule)
            {
                (side == RIGHT) ? setRightChainChangeCallback(targetModule) : setLeftChainChangeCallback(targetModule);
                //DEBUG("Setting callback for %s", searchModule->model->name.c_str());
                return targetModule;
            }
            keepSearching = searchModule && std::find(allowedModels.begin(), allowedModels.end(), searchModule->model) != allowedModels.end();
        }
        return nullptr;
    }

    T *module;

private:
    bool expanderUpdate = true;
    dsp::Timer expanderUpdateTimer;

    void onLeftChainChange(const ModuleX::ChainChangeEvent &e)
    {
        module->updateLeftExpanders();
    };
    void onRightChainChange(const ModuleX::ChainChangeEvent &e)
    {
        module->updateRightExpanders();
    };

    void setLeftChainChangeCallback(ModuleX *expander)
    {
        expander->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
        {
            this->onLeftChainChange(e);
        };
    };

    void setRightChainChangeCallback(ModuleX *expander)
    {
        expander->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
        {
            this->onRightChainChange(e);
        };
    };
};