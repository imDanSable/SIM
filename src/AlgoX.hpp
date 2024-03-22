
#include "biexpander/biexpander.hpp"

struct AlgoX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    AlgoX() = default;
    ;

   private:
    friend struct AlgoXWidget;
};

class AlgoXAdapter : public biexpand::BaseAdapter<AlgoX> {
   private:
   public:
};
