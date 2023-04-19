#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "constants.hpp"
#include "Connectable.hpp"
using namespace rack;
using namespace std;
using namespace constants;


class ModuleX : public Module, public Connectable
{
public:
	ModuleX(const ModelsListType& leftAllowedModels, 
		const ModelsListType& rightAllowedModels,
		std::function<void(float)> leftLightOn,
		std::function<void(float)> rightLightOn);
	~ModuleX();
	void onExpanderChange(const engine::Module::ExpanderChangeEvent &e) override;

	struct ChainChangeEvent { };
	std::function<void(const ChainChangeEvent& e)> chainChangeCallback = nullptr;

private:
	// template <typename Derived>
	// // XXX TODO we should use these consumesLeft and rightward, instead of the allowed models since the rules for consuming and allowing differ
	// const ModelsListType &getConsumesModules(sideType side) const
	// {
	// 	return (side == LEFT) ? static_cast<const Derived *>(this)->consumesLeftward : static_cast<const Derived *>(this)->consumesRightward;
	// }
};
