# SIM

![image](https://user-images.githubusercontent.com/127059186/223539676-4d2e4133-122c-4c5a-bd2b-0a2be1c33a7e.png)
## Coerce and Coerce6
| Coerce  | Coerce6 |
| ------------- | ------------- |
| ![image](https://user-images.githubusercontent.com/127059186/223539861-9bf2cf13-f8e1-46f4-9e12-4c758f7d1442.png)  | ![image](https://user-images.githubusercontent.com/127059186/223540406-f1f176dd-81e9-41d9-be84-db01877edc13.png)  |


Unlike traditional quantizers where the input is quantized to certain scales, Coerce uses the values from the polyphonic CV input (labeled **quant.**) as its quantize values. In order to have Coerce operate like a traditional quantizer, you can feed it a polyphonic signal with 7 channels where each channel carries one note pitch of that scale.

Since Coerce is not limited to scales, you can feed it an array of up to 16 of arbitrary values for quantization. 

Coerce is fully polyphonic so all incomming channels will be quantized to the corresponding channel on the output.

There are three rounding methods (accessible via the menu)

**Up**, **Closest** and **Down**.

There are two quantization modes (accessible via the menu)

 - **Octave fold:**
This is the default mode. In this mode quantization is folded around octaves (1 Volt).

 - **Restrict:**
This mode quantizes the inputs to the quantize value according to the selected rounding method.

Coerce6 is six versions of Coerce in one module. One on each row.
The quantize inputs are normalled down the middle column.
