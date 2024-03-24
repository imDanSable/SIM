#pragma once
#include <rack.hpp>
#include "biexpander/biexpander.hpp"

struct DebugX : biexpand::RightExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_1, OUTPUT_2, OUTPUT_3, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    DebugX()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configOutput(OUTPUT_1, "1");
        configOutput(OUTPUT_2, "2");
        configOutput(OUTPUT_3, "3");
    };

   private:
    friend struct DebugXWidget;
};

class DebugXAdapter : public biexpand::BaseAdapter<DebugX> {
   private:
   public:
    void setPort1(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[DebugX::OUTPUT_1].setVoltage(value, channel); }
    }
    void setPort2(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[DebugX::OUTPUT_2].setVoltage(value, channel); }
    }
    void setPort3(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[DebugX::OUTPUT_3].setVoltage(value, channel); }
    }

    void setChannels(int channels)
    {
        if (ptr) {
            ptr->outputs[DebugX::OUTPUT_1].setChannels(channels);
            ptr->outputs[DebugX::OUTPUT_2].setChannels(channels);
            ptr->outputs[DebugX::OUTPUT_3].setChannels(channels);
        }
    }

    bool inPlace(int /*length*/, int /*channel*/) const override
    {
        return true;
    }
};
