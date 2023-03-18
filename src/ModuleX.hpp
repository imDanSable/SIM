#pragma once
#include <rack.hpp>
#include <map>
#include "plugin.hpp"
#include "set"
#include "constants.hpp"
using namespace rack;
using namespace std;
using namespace constants;
class ModuleX : public Module
{
public:
    ModuleX();
    void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override;
	void onRemove(const RemoveEvent& e) override;

    Module* getModel(const Model* model, const sideType side);
    ModuleX *addAllowedModel(Model *model, sideType side);
    ModuleX *setLeftLightOn(std::function<void(float)> lightOn);
    ModuleX *setRightLightOn(std::function<void(float)> lightOn);

private:
    /// @brief <<ThisConsumes, GoingInThisDirection>, ConsumesThat>
    const map<const pair<const Model *, const sideType>, const Model *> consumeTable = {
        {make_pair(modelPhaseTrigg, RIGHT), modelReXpander}
    };
    ModuleX* getNextConnected(const sideType side);

    Module* lastLeftModule = nullptr;
    Module* lastRightModule = nullptr;

    /// @brief In case of direct communication between modules we need to check this if the module is still valid for write/read.
    bool beingRemoved = false;

    /// @brief Cached, so we can handle connection lights on behalf of ascendants
    function<void(float)> leftLightOn, rightLightOn;

    /// @brief What this module accepts on left and right side
    set<Model *> leftModels, rightModels;
};

