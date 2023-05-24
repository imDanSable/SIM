#include "InX.hpp"
#include "biexpander/biexpander.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

InX::InX()
{
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
};

json_t* InX::dataToJson()
{
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "insertMode", json_boolean(insertMode));
    return rootJ;
}
void InX::dataFromJson(json_t* rootJ)
{
    json_t* insertModeJ = json_object_get(rootJ, "insertMode");
    if (insertModeJ) { insertMode = json_boolean_value(insertModeJ); }
};

struct InXWidget : ModuleWidget {
    explicit InXWidget(InX* module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/InX.svg")));

        if (module) {
            module->addDefaultConnectionLights(this, InX::LIGHT_LEFT_CONNECTED,
                                               InX::LIGHT_RIGHT_CONNECTED);
        }

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 8; j++) {
                addInput(createInputCentered<SIMPort>(
                    mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module,
                    InX::INPUT_SIGNAL + (i * 8) + j));
            }
        }
    }

    void appendContextMenu(Menu* menu) override
    {
        InX* module = dynamic_cast<InX*>(this->module);
        assert(module);                     // NOLINT
        menu->addChild(new MenuSeparator);  // NOLINT
        menu->addChild(createBoolPtrMenuItem("Insert mode", "", &module->insertMode));
    }
};
Model* modelInX = createModel<InX, InXWidget>("InX");  // NOLINT