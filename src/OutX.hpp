
#include "ModuleX.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "plugin.hpp"

struct OutX : ModuleX
{

  friend struct OutXWidget;
  enum ParamId
  {
    PARAMS_LEN
  };
  enum InputId
  {
    INPUTS_LEN
  };
  enum OutputId
  {
    ENUMS(OUTPUT_SIGNAL, 16),
    OUTPUTS_LEN
  };
  enum LightId
  {
    LIGHT_LEFT_CONNECTED,
    LIGHT_RIGHT_CONNECTED,
    LIGHTS_LEN
  };

  OutX();
  void process(const ProcessArgs &args) override;
  // XXX Refactor this (and spike::process) so it doesn't return a bool
  bool setExclusiveOutput(int outputIndex, float value, int channel = 0);
  bool setOutput(int outputIndex, float value, int channel = 0);

  json_t *dataToJson() override;
  void dataFromJson(json_t *rootJ) override;

private:
  /// @brief The last output index that was set to a non-zero value per channel
  std::array<int, constants::NUM_CHANNELS> lastHigh = {};
  bool normalledMode = false;
  bool snoopMode = false;
};

using namespace dimensions; // NOLINT
struct OutXWidget : ModuleWidget
{
  explicit OutXWidget(OutX *module)
  {
    const auto width = 4 * HP;
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/OutX.svg")));

    addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec((X_POSITION_CONNECT_LIGHT), Y_POSITION_CONNECT_LIGHT)), module, OutX::LIGHT_LEFT_CONNECTED));
    addChild(createLightCentered<TinySimpleLight<GreenLight>>(mm2px(Vec(width - X_POSITION_CONNECT_LIGHT, Y_POSITION_CONNECT_LIGHT)), module, OutX::LIGHT_RIGHT_CONNECTED));
    int id = 0;
    for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        addOutput(createOutputCentered<SIMPort>(mm2px(Vec((2 * i + 1) * HP, JACKYSTART + (j)*JACKYSPACE)), module, id++));
      }
    }
  }

  void appendContextMenu(Menu *menu) override
  {
    OutX *module = dynamic_cast<OutX *>(this->module);
    assert(module);                    // NOLINT
    menu->addChild(new MenuSeparator); // NOLINT
    menu->addChild(createBoolPtrMenuItem("Normalled Mode", "", &module->normalledMode));
    menu->addChild(createBoolPtrMenuItem("Snoop Mode", "", &module->snoopMode));
  }
};
