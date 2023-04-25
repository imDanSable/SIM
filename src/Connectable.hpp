#pragma once
#include "plugin.hpp"
#include <functional>
#include <utility>
#include <vector>

// XXX Perhaps use CRTP to have access to derived so checklight knows its derived
class Connectable
{
public:
  using ModelsListType = std::vector<Model *>;
  Connectable(std::function<void(float)> leftLightOn, std::function<void(float)> rightLightOn, ModelsListType leftAllowedModels, ModelsListType rightAllowedModels)
      : leftAllowedModels(std::move(leftAllowedModels)), rightAllowedModels(std::move(rightAllowedModels)), leftLightOn(std::move(leftLightOn)), rightLightOn(std::move(rightLightOn)){};
  /// @brief Call from derived class's onExpanderChange() method.
  void checkLight(bool side, const Module *module, const std::vector<Model *> &allowedModels);

  const ModelsListType leftAllowedModels;  // NOLINT
  const ModelsListType rightAllowedModels; // NOLINT

private:
  std::function<void(float)> leftLightOn, rightLightOn;
};