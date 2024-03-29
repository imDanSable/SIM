#pragma once

#include "../HCVFunctions.h"
#include "Gamma/Domain.h"
#include "Gamma/scl.h"
#include "HCVPhasorCommon.h"
#include <rack.hpp>
class HCVPhasorSlopeDetector {
   public:
    float operator()(float _normalizedPhasorIn)
    {
        return calculateSteadySlope(_normalizedPhasorIn);
    }

    float calculateSteadySlope(float _normalizedPhasorIn)
    {
        slope = _normalizedPhasorIn - lastSample;
        lastSample = _normalizedPhasorIn;
        return gam::scl::wrap(slope, 0.5f, -0.5f);
    }

    float calculateRawSlope(float _normalizedPhasorIn)
    {
        slope = _normalizedPhasorIn - lastSample;
        lastSample = _normalizedPhasorIn;
        return slope;
    }

    float getSlopeInHz()
    {
        return slope * gam::Domain::master().spu();  // multiply slope by sample rate
    }

    float getSlopeInBPM()
    {
        return getSlopeInHz() * 60.0f;
    }

    float getSlopeDirection()
    {
        if (slope > 0.0f) return 1.0f;
        if (slope < 0.0f) return -1.0f;
        return 0.0f;
    }

    float getSlope()
    {
        return slope;
    }

    bool isPhasorAdvancing()
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
        return std::abs(slopeDetector.calculateRawSlope(_normalizedPhasorIn)) >= threshold;
    }

    void setThreshold(float _threshold)
    {
        threshold = clamp(_threshold, 0.0f, 1.0f);
    }

   private:
    float lastSample = 0.0f;
    float threshold = 0.5f;
    HCVPhasorSlopeDetector slopeDetector;
    rack::dsp::BooleanTrigger repeatFilter;
};

class HCVPhasorStepDetector {
   public:
    // bool operator()(float _normalizedPhasorIn);
    bool operator()(float _normalizedPhasorIn);

    bool addDeltaPhase(float _deltaPhase)
    {
        return this->operator()(_deltaPhase +
                                (currentStep + offset) / static_cast<float>(numberSteps));
    }

    int getCurrentStep() const
    {
        return ((currentStep + offset) % numberSteps + offset) % maxSteps;
    }
    void setNumberSteps(int _numSteps)
    {
        numberSteps = std::max(1, _numSteps);
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

    int advanceStep()
    {
        bool endOfCycle = false;
        if (currentStep == numberSteps - 1) { endOfCycle = true; }
        currentStep = (currentStep + 1) % numberSteps;
        fractionalStep = 0.0f;
        stepChanged = true;
        eoc = endOfCycle;
        assert(currentStep >= 0 && currentStep < numberSteps);
        return currentStep;
    }

    bool getEndOfCycle() const
    {
        return eoc;
    }

   private:
    int currentStep = 0;
    int numberSteps = 1;
    int offset = 0;
    int maxSteps = std::numeric_limits<int>::max();
    bool stepChanged = false;
    bool eoc = false;
    bool isPlaying = false;
    float fractionalStep = 0.0f;
    HCVPhasorResetDetector stepresetDetector;
    HCVPhasorResetDetector cycleResetDetector;
    HCVPhasorSlopeDetector slopeDetector;
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

    float getBasicGate(float _normalizedPhasor)
    {
        return _normalizedPhasor < gateWidth ? HCV_PHZ_GATESCALE : 0.0f;
    }

    float getSmartGate(float _normalizedPhasor);

    float operator()(float _normalizedPhasor)
    {
        if (smartMode) return getSmartGate(_normalizedPhasor);
        return getBasicGate(_normalizedPhasor);
    }

   private:
    float gateWidth = 0.5f;
    HCVPhasorSlopeDetector slopeDetector;
    bool smartMode = false;
    bool reversePhasor = false;
};