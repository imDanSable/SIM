#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "Connectable.hpp"
#include "constants.hpp"

template <typename T>
class Expandable : public Module, Connectable
{
public:
    Expandable(const ModelsListType &leftAllowedModels, const ModelsListType &rightAllowedModels,
               std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn) : Connectable(leftLightOn, rightLightOn, leftAllowedModels, rightAllowedModels),
                                                                                                  module(static_cast<T *>(this)){};
	void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override
    {
        checkLight(e.side, module, e.side ? rightAllowedModels : leftAllowedModels);
        if (e.side)
        {
            if (rightExpander.module)
            {
                ModuleX* exp = dynamic_cast<ModuleX*>(rightExpander.module);
                if (exp)
                {
                    exp->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
                    {
                        this->onRightChainChange(e);
                    };
                }
            }
        } else {
            if (leftExpander.module)
            {
                ModuleX* exp = dynamic_cast<ModuleX*>(leftExpander.module);
                if (exp)
                {
                    exp->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
                    {
                        this->onLeftChainChange(e);
                    };
                }
            }
        }
    }
protected:
    // XXX TODO templatize left/right
    template <typename M>
    M* updateCachedExpanderModule(sideType side, const ModelsListType &allowedModels)
    {
        Module* searchModule = module;
        bool keepSearching = true;
        if (side == LEFT)
        {
            if (!module->leftExpander.module)
                return nullptr;
            while (keepSearching)
            {
                searchModule = searchModule->leftExpander.module;
                if (!searchModule)
                    break;
                ModuleX* modulex = dynamic_cast<ModuleX*>(searchModule);
                if (modulex)
                    modulex->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
                    {
                        this->onLeftChainChange(e);
                    };
                keepSearching = std::find(allowedModels.begin(), allowedModels.end(), searchModule->model) != allowedModels.end();
            }
            return dynamic_cast<M *>(searchModule);
        }
        else
        {
            if (!module->rightExpander.module)
                return nullptr;
            while (keepSearching)
            {
                searchModule = searchModule->rightExpander.module;
                if (!searchModule)
                    break;

                ModuleX* modulex = dynamic_cast<ModuleX*>(searchModule);
                if (modulex)
                    modulex->chainChangeCallback = [this](const ModuleX::ChainChangeEvent &e)
                    {
                        this->onRightChainChange(e);
                    };
                keepSearching = searchModule && std::find(allowedModels.begin(), allowedModels.end(), searchModule->model) != allowedModels.end();
            }
            return dynamic_cast<M *>(searchModule);
        }
        return nullptr;
    }
    T *module;

private:
    bool expanderUpdate = true;
    dsp::Timer expanderUpdateTimer;

    void onLeftChainChange(const ModuleX::ChainChangeEvent &e)
    {
        module->updateLeftCachedExpanderModules();
    };
    void onRightChainChange(const ModuleX::ChainChangeEvent &e)
    {
        module->updateRightCachedExpanderModules();
    };
};