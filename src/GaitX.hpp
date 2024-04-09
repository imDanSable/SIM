#pragma once
#include <rack.hpp>
#include "biexpander/biexpander.hpp"

struct GaitX : biexpand::BiExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_EOC, OUTPUT_PHI, OUTPUT_STEP, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHTS_LEN };

    GaitX() : BiExpander(true)
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configOutput(OUTPUT_EOC, "End of Cycle");
        configOutput(OUTPUT_PHI, "Step Phase");
        configOutput(OUTPUT_STEP, "Current Step");
        configCache();
    };

    enum StepOutputVoltageMode {
        SCALE_10V_TO_16STEPS,
        SCALE_10_TO_LENGTH_PLUS,
        SCALE_10_TO_LENGTH
    };
    StepOutputVoltageMode stepOutputVoltageMode =     // NOLINT
        StepOutputVoltageMode::SCALE_10V_TO_16STEPS;  // NOLINT

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "stepOutputVoltageMode",
                            json_integer(static_cast<int>(stepOutputVoltageMode)));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override
    {
        json_t* stepOutputVoltageModeJ = json_object_get(rootJ, "stepOutputVoltageMode");
        if (stepOutputVoltageModeJ) {
            stepOutputVoltageMode =
                static_cast<StepOutputVoltageMode>(json_integer_value(stepOutputVoltageModeJ));
        }
    }

   private:
    friend struct GaitXWidget;
};

class GaitXAdapter : public biexpand::BaseAdapter<GaitX> {
   private:
   public:
    void setEOC(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[GaitX::OUTPUT_EOC].setVoltage(value, channel); }
    }
    void setPhi(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[GaitX::OUTPUT_PHI].setVoltage(value, channel); }
    }
    // void setStepVoltage(float value, int channel = 0)
    // {
    //     if (ptr) { ptr->outputs[GaitX::OUTPUT_STEP].setVoltage(value, channel); }
    //     // XX implement options for scaling
    // }
    void setStep(int step, int totalSteps, int channel = 0)
    {
        if (ptr) {
            switch (ptr->stepOutputVoltageMode) {
                case GaitX::SCALE_10V_TO_16STEPS:
                    ptr->outputs[GaitX::OUTPUT_STEP].setVoltage(0.625f * step, channel);
                    break;
                case GaitX::SCALE_10_TO_LENGTH_PLUS:
                    ptr->outputs[GaitX::OUTPUT_STEP].setVoltage(
                        rescale(static_cast<float>(step), 0.F,
                                static_cast<float>(totalSteps - (totalSteps > 0)), 0.F, 10.F));
                    break;
                case GaitX::SCALE_10_TO_LENGTH:
                    ptr->outputs[GaitX::OUTPUT_STEP].setVoltage(10.F * step / totalSteps, channel);
                    break;
            }
        }
    }

    bool getEOCConnected() const
    {
        return ptr && ptr->outputs[GaitX::OUTPUT_EOC].isConnected();
    }
    void setChannels(int channels)
    {
        if (ptr) {
            ptr->outputs[GaitX::OUTPUT_EOC].setChannels(channels);
            ptr->outputs[GaitX::OUTPUT_PHI].setChannels(channels);
            ptr->outputs[GaitX::OUTPUT_STEP].setChannels(channels);
        }
    }

    bool getPhiConnected() const
    {
        return ptr && ptr->outputs[GaitX::OUTPUT_PHI].isConnected();
    }
    bool getStepConnected() const
    {
        return ptr && ptr->outputs[GaitX::OUTPUT_STEP].isConnected();
    }
    bool inPlace(int /*length*/, int /*channel*/) const override
    {
        return true;
    }
};
