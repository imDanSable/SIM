
#include "biexpander/biexpander.hpp"
#include "plugin.hpp"

struct GlideX : biexpand::LeftExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    GlideX(){};

   private:
    friend struct GlideXWidget;
};

class GlideXAdapter : public biexpand::BaseAdapter<GlideX> {
   private:
   public:
};
