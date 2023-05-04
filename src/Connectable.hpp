#pragma once
#include <functional>
#include <utility>
#include <vector>
#include "plugin.hpp"
#include "widget/Widget.hpp"
// XXX Fix light for modulex using expandable's list and searching for oneself.

/// @brief Base class for Expandable modules and ModuleX modules (expanders).
/// @details Takes care of the connection lights. No need to interact with this class directly.

class Connectable : public Module {
   public:
    using ModelsListType = std::vector<Model*>;
    explicit Connectable(int leftLightId = -1,
                         int rightLightId = -1,
                         const ModelsListType& leftAllowedModels = {},
                         const ModelsListType& rightAllowedModels = {})
        : leftAllowedModels(leftAllowedModels),
          rightAllowedModels(rightAllowedModels),
          leftLightId(leftLightId),
          rightLightId(rightLightId){};
    /// @brief Call from derived class's onExpanderChange() method.
    bool checkLight(bool side, const Module* module, const std::vector<Model*>& allowedModels);
    void addConnectionLights(Widget* widget);

    const ModelsListType leftAllowedModels;   // NOLINT
    const ModelsListType rightAllowedModels;  // NOLINT

    int getRightLightId() const
    {
        return rightLightId;
    }
    int getLeftLightId() const
    {
        return leftLightId;
    }
    void setRightLightBrightness(float brightness)
    {
        if (rightLightId >= 0) { lights[rightLightId].setBrightness(brightness); }
    }
    void setLeftLightBrightness(float brightness)
    {
        if (leftLightId >= 0) { lights[leftLightId].setBrightness(brightness); }
    }

   private:
    int leftLightId;
    int rightLightId;
};