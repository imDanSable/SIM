#pragma once
#include <functional>
#include <utility>
#include <vector>
#include "plugin.hpp"
#include "widget/Widget.hpp"

/// @brief Base class for Expandable modules and ModuleX modules (expanders).
/// @details Takes care of the connection lights. No need to interact with this class directly.

class Connectable : public Module {
   public:
    using ModelsListType = std::vector<Model*>;
    Connectable(int leftLightId,
                int rightLightId,
                const ModelsListType& leftAllowedModels,
                const ModelsListType& rightAllowedModels)
        : leftAllowedModels(leftAllowedModels),
          rightAllowedModels(rightAllowedModels),
          leftLightId(leftLightId),
          rightLightId(rightLightId){};
    /// @brief Call from derived class's onExpanderChange() method.
    void checkLight(bool side, const Module* module, const std::vector<Model*>& allowedModels);
    void addConnectionLights(Widget* widget);

    const ModelsListType leftAllowedModels;   // NOLINT
    const ModelsListType rightAllowedModels;  // NOLINT

   private:
    int leftLightId;
    int rightLightId;
};