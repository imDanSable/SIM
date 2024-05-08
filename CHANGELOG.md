# 2.1.1
Release date: 

- Updates:
    - Added modules:
      - Gmod (Gate Modifier) 
      - Crd (Chord Sequencer / Polyphonic switch)
- Changes:
    - Spike and Phi have an option to respond to negative NEXT triggers for reverse direction
    - Tiny switches now glow in the dark
    - Minor graphical improvements
    - Out<sup>x</sup> now defaults to normalled
    - Added Switch tag to Phi
- Fixed:
    - [issue #5](https://github.com/imDanSable/SIM/issues/5) Out<sup>x</sup> would output data beyond the end of a shortened data buffer.
    - [issue #4](https://github.com/imDanSable/SIM/issues/4) Segment of Arr<sup>x</sup> now only reflects shortening from input adapters and not from output adapters for clarity.

# 2.1.0
release date: April 12, 2024

- Updates:
    - Added modules:
      - Phi I(Sequencer)
      - Spike (Gate Sequencer)
      - Re<sup>x</sup> (Expander)
      - In<sup>x</sup> (Expander)
      - Out<sup>x</sup> (Expander)
      - Via (Expandable)
      - Bank (Value provider)
      - Arr (Value provider)
      - Tie (Legato)

# 2.0.1

Release date: February 7, 2024

- Fixed: 
    - When input 'quant' is not connected, the output will be equal to the input instead of zeros.

# 2.0.0

Release date: March 10, 2024

Initial release.
