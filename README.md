# SIM <a name="SIM"></a>

<img src="https://user-images.githubusercontent.com/127059186/228753963-1d831a97-dd7d-4ae0-85c7-96da431e109e.png" height="300">
-----


## <a name="coerce"></a> Coerce

**TL;DR** Coerce quantizes a polyphonic signal to the values of another polyphonic signal.

<img src="https://user-images.githubusercontent.com/127059186/227324482-7a25272e-523e-445b-a8f8-dee1464c4019.png" height="400">


<ins>**in**:</ins> The voltages of the polyphonic input at *in* in will be quantized.

<ins>**quantize**:</ins>  (quant.) The voltages of the polyphonic input at *quant.* will be used as quantization values. 

<ins>**out**:</ins> Outputs the quantized voltages using. The number of channels at **out** is the same as the number of input channels at **in**. The order of the channels is left unchanged. 

<ins>**Operation mode**:</ins> The menu allows you to choose between two operation modes. The default is *Octave fold*. In this musical mode you can imagine the quantization values of *quant.* being copied to all octaves before the input is quantized. In the second mode, *Restrict*, each input value is quantized against the voltages coming in at *quant.* without any regard for octaves.

<ins>**Rounding method**:</ins> The menu allows you to choose between three rounding methods for quantization: *Up*, *Closest*, and *Down*.

*Tip:* To use Coerce musically you could feed it quantization values from modules that output a chord or a scale like [Chords](https://library.vcvrack.com/dbRackSequencer/Chords), [ChordCV](https://library.vcvrack.com/AaronStatic/ChordCV).
Unlike traditional quantizers where the input is quantized to certain scales, Coerce can use an array of arbitrary values. On top of that the voltages can vary over time. As quantization takes place at audio rates, Coerce can be used as an audio effect as well. 


## <a name="coerce6"></a> Coerce6 
<img src="https://user-images.githubusercontent.com/127059186/227324741-20c98dde-7912-493c-8874-d7a421109472.png" height="400"/> 

Coerce6 is six versions of Coerce in one module. One on each row.
The quantize inputs (the middle column) are normalled down the middle column allowing.



## <a name="re-x"></a>Reˣ

Reˣ is an expander whose function depends on which compatible module it is placed next to and on which side. 

<img src="https://user-images.githubusercontent.com/127059186/227324993-f343b338-b39b-42fe-877f-93d742d52a1d.png" height="400"/>

Compatible modules: 
 - [Reˣ-Spike+](#rex-spike)



## <a name="spike"></a>Spike

**TL;DR** Spike is 16 channel, 16 step, voltage addressable gate sequencer with adjustable start positions and sequence lengths per channel and gate duration control per step.

<image src="https://user-images.githubusercontent.com/127059186/228751624-0c677480-a359-413e-b688-d2e33cbc7be5.png" height="400"/>


<ins>**Φ-in**:</ins> The voltage at *Φ-in* is the address selector for the gate sequencer. In the default mode, 0 Volt to 0.625 Volt selects the first step, 0.625 to 1.25 the second, etc. 0.625 is simply 10 Volt divided by 16 Steps. A sawtooth wave ranging from 0 to 10 Volt at *Φ-in* would play all sixteen gates sequentially. Or in other words: the voltage at *Φ-in* drives an imaginary playhead. This also makes it possible to jump the playhead to another location in zero time, or to play the gate sequence in reverse feeding it the right input. A gate step is selected as soon as a step boundary is reached in either direction. When the playhead jumps over one ore more steps, those gate steps will not be selected.
 If more than one channel is fed into *Φ-in* Spike becomes polyphonic. The number of parallel running sequencers is equal to the number of *Φ-in* channels. 

<ins>**1-8** and **9-16**:</ins> The buttons area has the purposes:
 - 1. Programming the sequencer: Clicking the buttons allows you to toggle the memory locations at the corresponding addresses of the sequencer. A white light indicates the gate memory state is *on*. A gray light indicates the memory state is *on*, but the step lies outside the range of the sequence. A dark button indicates the memory state is *off*
 - 2. Show the start and end of the sequence: They are indicated by a light blue rounded background at both the endings of a pinkish line.
 - 3. To indicate the current step: A white circle around the current gate button.

<ins>**Operation mode**:</ins> Using the context menu you can choose if all parallel sequencers share a single gate memory bank (bank 1), or if each sequencer has its own gate memory bank.

<ins>**edit**:</ins> With the *edit* knob you select which of the 16 channels to edit (gate buttons) and monitor (active step and start/length indicators). If the operation mode is one bank per *Φ-in*, the edit button will only select up to the number of *Φ-in* channels.



**duration** (dur.) The duration input is used to control gate lengths. There are two different modes to control gate duration. Absolute duration and relative duration.
 - For absolute duration you can select one of the three gate duration ranges from the context menu: 1 ms to 100 ms, 1 ms to 1000 ms and 1 ms to 10 seconds. 
 - With relative mode enabled from the menu the playhead speed and position determine whether the gate should be high or low. The duration knob controls the relative gate length in percentage of a single *step*. When a step boundary is crossed causing a gate to go high, it will stay high until the voltage the playhead arrives at to the set percentage of that step. When the *Φ-in* is falling, or in other words when entering a step from above, it will stay high for the same range as it would if coming from below. A side effect in relative mode is that if you set the *duration* to 100% and toggle multiple gates that are next to each other *on*, you create a single longer gate. You can easily create ostinatos and musical rhythmic patterns by dividing a measure into 8 for example (or 12 if you want to have triplets) and filling it with *on*s and *off*s.

When connecting a cable to the CV of the duration parameter with only a single voltage, all gates will have the same absolute or relative duration. The *duration* knob becomes an attenuator once the CV input is connected.

When you connect a polyphonic cable to the CV input of the *duration* parameter, each channel controls the duration of a the corresponding step. Therefore it is advisable to have at least as many channels connected to the CV input as the sequence length. Otherwise, as prescribed by the VCV Rack standards for polyphony, the steps for which there is no corresponding duration input will receive 0 volts and will effectively become  1 ms. triggers. 



## <a name="rex-spike"></a>Reˣ + Spike 

By placing the Re-Expander [Reˣ](#re-x) left of Spike, you can control both the start point and the length of the sequence. 

The knobs *start* and *length* control the start and length for all parallel sequences. Note: When using a a phase clock (saw tooth) to drive the playhead, shortening the sequence length, slows down the sequence. 

When using a polyphonic signal for the CV *start* or *length* input, each channel controls the start and length of the corresponding parallel sequence. 

Note: The *start* and *length* knobs do not become attenuators when a cable is connected to the CV input as these knobs snap to discrete values.

