# SIM
![image](https://user-images.githubusercontent.com/127059186/227324264-30cdd4ee-0668-45f2-8fd6-45e0e2ec94b9.png)
## Coerce
![image](https://user-images.githubusercontent.com/127059186/227324482-7a25272e-523e-445b-a8f8-dee1464c4019.png)


**TL;DR;** Coerce quantizes a polyphonic signal to the values of another polyphonic signal.

**in**

The voltages of the polyphonic input at *in* in will be quantized.

**quant.**

The voltages of the polyphonic input at *quant.* will be used as quantization values. 

**out**

Outputs the quantized voltages using. The number of channels at **out** is the same as the number of input channels at **in**. The order of the channels is left unchanged. 

**Operation mode**

The menu allows you to choose between two operation modes. The default is *Octave fold*. In this musical mode, you can imagine the quantization values of *quant.* being copied to all octaves before the input is quantized. In the second mode, *Restrict*, each input value is quantized against the voltages coming in at *quant.* without any regard for octaves.

**Rounding method**

The menu allows you to choose between three rounding methods for quantization: *Up*, *Closest*, and *Down*.

--------------
To use Coerce musically you could feed it quantization values from modules that output a chord or a scale like [Chords](https://library.vcvrack.com/dbRackSequencer/Chords), [ChordCV](https://library.vcvrack.com/AaronStatic/ChordCV).
Unlike traditional quantizers where the input is quantized to certain scales, Coerce can use an array of arbitrary values. On top of that, the voltages can vary over time. As quantization takes place at audio rates, Coerce can be used as an audio effect as well. 

## Coerce6 
![image](https://user-images.githubusercontent.com/127059186/227324741-20c98dde-7912-493c-8874-d7a421109472.png)

Coerce6 is six versions of Coerce in one module. One on each row.
The quantize inputs (the middle column) are normalled down the middle column allowing.


------------------

## <a name="re-x"></a>Reˣ
![image](https://user-images.githubusercontent.com/127059186/227324993-f343b338-b39b-42fe-877f-93d742d52a1d.png)

Reˣ is an expander whose function depends on which compatible module it is placed next to and on which side. Currently Reˣ is only compatible with [Spike](#spike) placed on the left side. For its functionality see the documentation [Spike+Reˣ](#spike-rex)

## <a name="spike"></a>Spike
![image](https://user-images.githubusercontent.com/127059186/227329070-e745054e-e7fa-4af3-99f7-a78ce94bbcd4.png)

**TL;DR;** Spike is 16 step (times 16 channels) addressable gate sequencer with adjustable start positions and sequence lengths per channel and gate duration control per step.

**Φ-in**

The voltage at *Φ-in* is the address selector for the gate sequencer. In the default mode (without a compatible expander and with *pattern* disconnected), 0 Volt to 0.625 Volt selects the first step. 0.625 is simply 10Volt/16 Steps. Each 0.625 voltage increase selects the next step. A sawtooth at *Φ-in* would play all sixteen gates sequentially. The voltage at *Φ-in* drives an imaginary playhead, making it possible to jump the playhead to another location in zero time, or to play the gate sequence in reverse. If *Φ-in* is polyphonic, the number of parallel running sequencers is equal to the number of *Φ-in* channels. Gates will go high in both directions. Whether *Φ-in* is rising or falling, as soon as a *step* boundary is crossed, that gate step is selected. Except when a sudden jump is big enough to cause the playhead to skip one or more steps. Then the skipped steps will not be selected and thus not go high.

**1-8** and **9-16**

The buttons serve three purposes:
 - Clicking the buttons allows you to toggle the memory locations at the corresponding addresses of the sequencer. A white light indicates the gate memory state is *on*. A gray light indicates the memory state is *on*, but the *step* lies outside the range of the sequence. When Spike is used polyphonically, all parallel sequencers will use the same memory bank for the gate states (although they can be manipulated to differ using [Reˣ](#re-x)).
 - Show the start and end of the sequence. Without using expanders the first button will always be the start, indicated by a green light, and the last button the end, indicated by a red light. When using Spike polyphonically only the start and end of the first channel will be indicated using green and red button lights for simplicity even though each channel can have a different start and end point.
 - To indicate the current step, or in other words, the position of the playhead. A small blue dot in the middle of the buttons will light up when that step is active. When using Spike polyphonically each parallel sequencer will have its own indicator.

**pattern** (patt.)

The pattern input port lets you manipulate the sequencer's memory using voltages. With *pattern* connected, channel 1 to 16 control step 1 to 16. A positive voltage will set the corresponding gate memory to *on*. Zero or negative voltages will turn the step *off*. Connecting a cable to *pattern* overrides the memory as set by the buttons. The length of the sequence is equal to the number of channels connected to *pattern*.   

**duration** (dur.)

The duration input is used to manipulate gate lengths. There are two different modes to control gate duration. Absolute duration and relative duration.
 - For absolute duration you can select one of the three gate duration ranges from the context menu: 1 ms to 100 ms, 1 ms to 1000 ms and 1 ms to 10 seconds. 
 - With relative mode enabled from the menu, the play-head, driven by the *Φ-in* signal determines whether the gate should be high or low. The duration knob controls the relative gate length in the percentage of a single *step*. When the *Φ-in* signal causes a new gate to go high, it will be high until the voltage at *Φ-in* drives play-head to the set percentage of that step. A side effect of this is that if you set it to 100% you can concatenate gates and make long and short gates using adjacent buttons. Relative mode, like absolute mode, works in both directions. If *Φ-in* falls instead of rises, thus steering the playhead in the reverse direction through the sequencer memory, it would set the gate to high when it enters the gate step from above and stay high while the play-head is between to top address of the step boundary and the top address minus the percentage set by the duration knob.

When connecting a cable to the CV of the duration parameter with only a single voltage, all gates will have the same absolute or relative duration depending on the selected mode. With the CV connected, the duration knob becomes an attenuator.

But when you connect a polyphonic cable to the CV input of the duration parameter, each channel controls the duration of a different step. Therefore it is advisable to have at least as many channels connected to the CV input as the sequence length. Otherwise, as prescribed by the VCV Rack standards for polyphony, the steps for which there is no corresponding duration input will receive 0 volts and thus will effectively become triggers. 

## <a name="spike-rex"></a>Spike + Reˣ

By placing Re-Expander [Reˣ](#re-x) left of Spike, you can control both the start point and the length of the sequence. The *start* and *length* knobs do not become attenuators when a cable is connected to the CV input, as these knobs snap to discrete values.
When Spike is used polyphonically the parameter knobs *start* and *length* control the start and length for all parallel sequencers.
When using a polyphonic cable for the CV input, each channel controls the start and length of a corresponding parallel sequencer in Spike. 

If Reˣ is connected and the *pattern* input is used, thus giving the sequence a length equal to the number of channels connected to *pattern*, the parameter knobs *start* and *length* operate within the sequence length set by *pattern*. So it could be the case that when *pattern* enforces a sequence length of 7, dialing in a length longer than 7 using the *length* knob on Reˣ would have no effect.

