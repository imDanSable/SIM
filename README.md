# SIM

## Coerce
![Coerce](https://github.com/imDanSable/SIM/blob/master/coerce.png)

**Coerce** is a voltage quantizer that applies quantization to incoming voltages and outputs the adjusted voltages to the output port on the same row. 
The quantization values are read from the Quantize port. These Quantize ports are normalled, so the quantization values that are used are read from the same row as the in and output, or if that one is not connected, from the first connected Quantize port moving upwards.

There are two quantization modes that are accessible via the menu.
1) **Octave fold:**
This is the default mode and makes Coerce behave like a regular quantizer folding values around one Volt.
2) **Restrict:**
In this mode, all input voltages are quantized to quantize values.

There are three rounding methods that are accessible via the menu.
**Up**, **Closest** and **Down**.
