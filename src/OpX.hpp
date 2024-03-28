
#include "biexpander/biexpander.hpp"

struct OpX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    OpX() = default;  // Don't forget configDirtyFlags() in constructor

   private:
    friend struct OpXWidget;
};

class OpXAdapter : public biexpand::BaseAdapter<OpX> {
   private:
   public:
};
