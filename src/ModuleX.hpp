#pragma once
#include "Connectable.hpp"
#include "constants.hpp"
#include "plugin.hpp"
#include <functional>
#include <rack.hpp>
#include <utility>

class ModuleX : public Module, public Connectable
{
public:
  ModuleX(const ModelsListType &leftAllowedModels, const ModelsListType &rightAllowedModels, std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn);
  ModuleX(const ModuleX &other) = delete;
  ModuleX &operator=(const ModuleX &other) = delete;
  ModuleX(ModuleX &&other) = delete;
  ModuleX &operator=(ModuleX &&other) = delete;
  ~ModuleX() override;
  void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override;

  struct ChainChangeEvent
  {
  };
  using ChainChangeCallbackType = std::function<void(const ChainChangeEvent &e)>;

  void setChainChangeCallback(ChainChangeCallbackType callback) { chainChangeCallback = std::move(callback); }

  ChainChangeCallbackType getChainChangeCallback() const { return chainChangeCallback; }

private:
  ChainChangeCallbackType chainChangeCallback = nullptr;
};
