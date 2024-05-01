#pragma once

#include <rack.hpp>
#include "wrappers.hpp"

static constexpr float HCV_PHZ_UPSCALE = 10.0f;
static constexpr float HCV_PHZ_DOWNSCALE = 0.1f;
static constexpr float HCV_PHZ_GATESCALE = 10.0f;

/*
I'd like to thank Michael Hetrick for his work on the Phasor aware modules in VCV Rack.
This code is is almost a direct copy of files HCVPhasorAnalyzers.hpp
from his VCV plugin http:://github.com/mhetrick/hetrickcv
*/
class HCVPhasorSlopeDetector {
   public:
    float operator()(float _normalizedPhasorIn)
    {
        return calculateSteadySlope(_normalizedPhasorIn);
    }

    // float calculateSteadySlope(float _normalizedPhasorIn)
    // {
    //     slope = _normalizedPhasorIn - lastSample;
    //     lastSample = _normalizedPhasorIn;
    //     return wrappers::wrap(slope, 0.5f, -0.5f);
    // }

    float calculateSteadySlope(float _normalizedPhasorIn)
    {
        float difference = _normalizedPhasorIn - lastSample;
        if (std::abs(difference) > 0.5f) {
            // The phasor has wrapped around, so adjust the difference
            difference = 1.0f - std::abs(difference);
            // Adjust the sign of the difference based on the direction of the phasor
            if (_normalizedPhasorIn > lastSample) { difference = -difference; }
        }
        slope = difference;
        lastSample = _normalizedPhasorIn;
        return wrappers::wrap(slope, 0.5f, -0.5f);
    }

    float calculateRawSlope(float _normalizedPhasorIn)
    {
        float difference = _normalizedPhasorIn - lastSample;
        if (std::abs(difference) > 0.5f) {
            // The phasor has wrapped around, so adjust the difference
            difference = 1.0f - std::abs(difference);
            // Adjust the sign of the difference based on the direction of the phasor
            if (_normalizedPhasorIn > lastSample) { difference = -difference; }
        }
        slope = difference;
        lastSample = _normalizedPhasorIn;
        return slope;
    }
    // float calculateRawSlope(float _normalizedPhasorIn)
    // {
    //     slope = _normalizedPhasorIn - lastSample;
    //     lastSample = _normalizedPhasorIn;
    //     return slope;
    // }

    // float getSlopeInHz()
    // {
    //     return slope * gam::Domain::master().spu();  // multiply slope by sample rate
    // }

    // float getSlopeInBPM()
    // {
    //     return getSlopeInHz() * 60.0f;
    // }

    float getSlopeDirection() const
    {
        if (slope > 0.0f) { return 1.0f; }
        if (slope < 0.0f) { return -1.0f; }
        return 0.0f;
    }

    float getSlope() const
    {
        return slope;
    }

    bool isPhasorAdvancing() const
    {
        return std::abs(slope) > 0.0f;
    }

   private:
    float lastSample = 0.0f;
    float slope = 0.0f;
};

class HCVPhasorResetDetector {
   public:
    bool operator()(float _normalizedPhasorIn)
    {
        return detectProportionalReset(_normalizedPhasorIn);
    }

    bool detectProportionalReset(float _normalizedPhasorIn);

    bool detectSimpleReset(float _normalizedPhasorIn)
    {
        float slope = slopeDetector.calculateRawSlope(_normalizedPhasorIn);
        if (std::abs(slope) >= threshold) {
            lastResetSign = slope >= 0;
            return true;
        }
        return false;
    }
    bool getResetSign() const
    {
        return lastResetSign;
    }
    void setThreshold(float _threshold)
    {
        threshold = rack::math::clamp(_threshold, 0.0f, 1.0f);
    }

   private:
    bool lastResetSign = true;
    float lastSample = 0.0f;
    float threshold = 0.5f;
    HCVPhasorSlopeDetector slopeDetector;
    rack::dsp::BooleanTrigger repeatFilter;
};

class HCVPhasorStepDetector {
   public:
    bool operator()(float _normalizedPhasorIn);

    void reset()
    {
        numberSteps = 0;
        currentStep = 0;
        fractionalStep = 0.0f;
        stepChanged = false;
        eoc = false;
        loops = 0;
    }

    int getCurrentStep() const
    {
        return ((currentStep + offset) % numberSteps + offset) % maxSteps;
    }
    void setNumberSteps(int _numSteps)
    {
        numberSteps = std::max(1, _numSteps);
    }
    int getNumberSteps() const
    {
        return numberSteps;
    }
    void setOffset(int _offset)
    {
        offset = _offset;
    }
    void setMaxSteps(int _maxSteps)
    {
        maxSteps = _maxSteps;
    }
    /// @brief Returns how deep into the current step the phasor is
    float getFractionalStep() const
    {
        return fractionalStep;
    }
    bool getStepChangedThisSample() const
    {
        return stepChanged;
    }
    bool getIsPlaying() const
    {
        return isPlaying;
    }

    float fractionLeftToNextStep() const
    {
        return 1.0f - fractionalStep;
    }

    void setStep(int _step)
    {
        assert(currentStep >= 0 && currentStep < numberSteps);
        currentStep = _step;
        fractionalStep = 0.0f;
        stepChanged = true;
    }

    bool getEndOfCycle() const
    {
        return eoc;
    }

    bool getEOCSign() const
    {
        return cycleResetDetector.getResetSign();
    }

    int getLoops() const
    {
        return loops;
    }

   private:
    int currentStep = 0;
    int numberSteps = 0;
    int offset = 0;
    int loops = 0;
    int maxSteps = std::numeric_limits<int>::max();
    bool stepChanged = false;
    bool eoc = false;
    bool isPlaying = false;
    float fractionalStep = 0.0f;
    HCVPhasorResetDetector stepresetDetector;
    HCVPhasorResetDetector cycleResetDetector;
    // HCVPhasorSlopeDetector slopeDetector;
};

class HCVPhasorGateDetector {
   public:
    void setGateWidth(float _width)
    {
        gateWidth = _width;
    }
    void setSmartMode(bool _smartModeEnabled)
    {
        smartMode = _smartModeEnabled;
    }

    float getBasicGate(float _normalizedPhasor) const
    {
        return _normalizedPhasor < gateWidth ? HCV_PHZ_GATESCALE : 0.0f;
    }

    float getSmartGate(float _normalizedPhasor);

    float operator()(float _normalizedPhasor)
    {
        if (smartMode) { return getSmartGate(_normalizedPhasor); }
        return getBasicGate(_normalizedPhasor);
    }
    void setOffset(float _offset)
    {
        offset = _offset;
    }

   private:
    float offset = 0.0f;
    float previousPhasor = 0.0f;
    float gateWidth = 0.5f;
    HCVPhasorSlopeDetector slopeDetector;
    bool smartMode = false;
    bool reversePhasor = false;
};