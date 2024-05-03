// #include "plugin.hpp"

// void processInxIn(const ProcessArgs& args, int channels)
// {
// const bool trigOutConnected = outputs[TRIG_OUTPUT].isConnected();
// const bool cvOutConnected = outputs[OUTPUT_CV].isConnected();
// const int numSteps = inx.getLastConnectedInputIndex() + 1;
// bool eoc = false;
// if (numSteps == 0) { return; }
// for (int channel = 0; channel < channels; ++channel) {
//     const float curCv = inputs[INPUT_DRIVER].getNormalPolyVoltage(0.F, channel);
//     int curStep{};
//     if (usePhasor) {
//         curStep = getCurStep(curCv, numSteps);
//         if ((prevStepIndex[channel] != curStep)) {
//             if (trigOutConnected) { trigOutPulses[channel].trigger(gateLength); }
//             prevStepIndex[channel] = curStep;
//         }
//     }
//     else {  /// XXX Is this doing triggers using time after we switched to phasor? but
//     not
//             /// yet inx
//         const bool nextConnected = inputs[INPUT_NEXT].isConnected();
//         const bool ignoreClock = resetPulse.process(args.sampleTime);
//         const bool clockTrigger =
//             clockTracker[channel].process(args.sampleTime, curCv) && !ignoreClock;  //
//             NEXT
//         const bool triggered =
//             nextConnected
//                 ? nextTrigger.process(inputs[INPUT_NEXT].getVoltage()) && !ignoreClock
//                 : clockTrigger;
//         // REFACTOR
//         curStep = (prevStepIndex[channel] + triggered) % numSteps;
//         if (triggered) {
//             if (trigOutConnected) { trigOutPulses[channel].trigger(gateLength); }
//             prevStepIndex[channel] = curStep;
//         }
//     }
//     if (cvOutConnected) {
//         const int numChannels = inx.getChannels(curStep);
//         writeBuffer().resize(numChannels);
//         std::copy_n(iters::PortVoltageIterator(inx.getVoltages(prevStepIndex[channel])),
//                     numChannels, writeBuffer().begin());
//     }
//     processAuxOutputs(args, channel, numSteps, curStep, curCv, eoc);
// }
// }