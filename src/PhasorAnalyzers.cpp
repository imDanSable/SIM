#include "PhasorAnalyzers.hpp"
#include <cmath>

/*
I'd like to thank Michael Hetrick for his work on the Phasor aware modules in VCV Rack.
This code is is almost a direct copy of files HCVPhasorAnalyzers.cpp
from his VCV plugin http:://github.com/mhetrick/hetrickcv
*/

bool HCVPhasorResetDetector::detectProportionalReset(float _normalizedPhasorIn)
{
    const float difference = _normalizedPhasorIn - lastSample;
    const float sum = _normalizedPhasorIn + lastSample;
    lastSample = _normalizedPhasorIn;
    if (sum == 0.0f) { return false; }

    const float rawProportionalChange = difference / sum;
    const float proportionalChange = std::abs(rawProportionalChange);

    const bool resetDetected = proportionalChange > threshold;
    if (resetDetected) { lastResetSign = rawProportionalChange < 0.0f; }

    // return resetDetected;
    return repeatFilter.process(resetDetected);
}

/// @return stepChanged
bool HCVPhasorStepDetector::operator()(float _normalizedPhasorIn)
{
    float scaledPhasor = _normalizedPhasorIn * numberSteps;
    int incomingStep = floorf(scaledPhasor);
    fractionalStep = scaledPhasor - incomingStep;
    const bool resetDetected = stepresetDetector.detectSimpleReset(_normalizedPhasorIn);
    // const bool cycleReset = cycleResetDetector.detectSimpleReset(_normalizedPhasorIn);
    if (numberSteps == 1) {
        currentStep = 0;
        stepChanged = resetDetected;
        eoc = stepChanged;
        loops += eoc ? (stepresetDetector.getResetSign() ? 1 : -1) : 0;
        return stepChanged;
    }

    if (incomingStep != currentStep) {
        currentStep = incomingStep;
        assert(currentStep >= 0 && currentStep < numberSteps);
        stepChanged = true;
        eoc = resetDetected;
        loops += eoc ? (stepresetDetector.getResetSign() ? 1 : -1) : 0;
        return stepChanged;
    }

    stepChanged = false;
    eoc = false;
    return stepChanged;
}

float HCVPhasorGateDetector::getSmartGate(float normalizedPhasor)
{
    // only change reverse direction detection if phasor is moving, otherwise high frequency noise
    // will result ;) high frequency noise might still happen if monitoring via an "active" gate
    // this is likely happening because we are using 0-10V phasors, scaling them down to 0-1V
    // phasors, and scaling them up again. this scaling might be resulting in denormals that get
    // flushed to 0V, so very slow phasors might appear to have 0 slope.

    const float slope = slopeDetector(normalizedPhasor);
    const bool phasorIsAdvancing = slopeDetector.isPhasorAdvancing();
    if (phasorIsAdvancing) { reversePhasor = slope < 0.0f; }

    float gate = NAN;
    if (reversePhasor) {
        float adjustedPhasor = 1.0f - normalizedPhasor;
        gate = adjustedPhasor < gateWidth || adjustedPhasor > (1.0f - gateWidth) ? HCV_PHZ_GATESCALE
                                                                                 : 0.0f;
    }
    else {
        gate = normalizedPhasor < gateWidth || normalizedPhasor > (1.0f - gateWidth)
                   ? HCV_PHZ_GATESCALE
                   : 0.0f;
    }

    return gate;
}