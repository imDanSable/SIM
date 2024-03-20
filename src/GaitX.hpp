
#include "biexpander/biexpander.hpp"
#include "iters.hpp"
#include "plugin.hpp"

struct GaitX : biexpand::RightExpander {
   public:
    enum ParamId { PARAMS_LEN };
    enum InputId { INPUTS_LEN };
    enum OutputId { OUTPUT_EOC, OUTPUT_PHI, OUTPUT_STEP, OUTPUTS_LEN };
    enum LightId { LIGHT_LEFT_CONNECTED, LIGHT_RIGHT_CONNECTED, LIGHTS_LEN };

    GaitX()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configOutput(OUTPUT_EOC, "End of Cycle");
        configOutput(OUTPUT_PHI, "Step Phase");
        configOutput(OUTPUT_STEP, "Current Step");
    };

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
    void setStep(float value, int channel = 0)
    {
        if (ptr) { ptr->outputs[GaitX::OUTPUT_STEP].setVoltage(value, channel); }
        // XX implement options for scaling
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
    iters::BoolIter transform(iters::BoolIter first,
                              iters::BoolIter last,
                              iters::BoolIter out,
                              int channel) override
    {
        /// XXX FIX THIS to in place
        return std::copy(first, last, out);
    }

    iters::FloatIter transform(iters::FloatIter first,
                               iters::FloatIter last,
                               iters::FloatIter out,
                               int channel) override
    {
        /// XXX FIX THIS to in place
        return std::copy(first, last, out);
    }
};
