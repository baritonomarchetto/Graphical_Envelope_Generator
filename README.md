# Quad Graphical Envelope Generator Module for Eurorack Synthesizers
We are all used to segmented envelope generators, those (often but not always) 4-knobs modules with separate attack time (A), decay time (D), sustain level (S) and release time (R). These have been used for decades, work great and are a common device in any synthesizer.

They are not flawless, anyway (shame on meee!), the biggest limit being the flexibility of the voltage profile they can output.

Here it comes the idea: what if we could directly draw our envelope on a touchscreen and have voltages proportional to our drawings outputted?

# Hardware
The module is made of two boards: a main board hosting the digital and analog circuitry, and a front board for the touch screen and push buttons.

Main components have already been "revealed" in the previous step, with arduino nano 328p as the brain of the module, a MCP4728 QUAD DAC it's right arm and a 2.8" ST7789 screen with XPT2046 touch controller the "skin".

What else?

DAC's outputs are limited to 4V in origin, but here boosted to 8V with a TL074 in 2X gain, non inverting amplifier configuration. This extends the envelope generator operating voltage from 0 - 4V to 0 - 8V.
The amplifier acts also as DAC's outputs protection against over/inverse voltage.

Gates inputs are voltage protected via a series resitor and two zener diodes each.

To guarantee electrical connection between the touch screen and the faceplace, this is made of FR4 instead of aluminum. This gave me the opportunity to free some vital space on the main board by allocating the needed voltage dividers to the screen inputs directly on the faceplate (the screen is not 5V tolerant).

All power supply voltages are filtered with electrolitic and ceramic capacitors.

The module hosts four buttons, four output jacks (1/8 inch), four gate inputs (1/8 inch) and three potentiometers.

All components are cheap and common. Mono female jacks are those PJ301M you can buy everywhere online. Potentiometers are those most often labelled WH148. These are horizontal mounting pots, but by bending and "elongating" pins with some single pole conductor they can be easily mounted vertically. As a side effect, they give the module a vintage feeling that I like a lot :)

# Software
The main tasks of the code I wrote are:

- being able to store and visualize a user drawn envelope
- being able to output a voltage proportional to the instant envelope amplitude at any time
  
Four channels are handled and can be triggered at different, independent times.

The code makes use of different libraries from Arduino community:

- Adafruit_GFX.h - graphic library
- Adafruit_ST7789.h - Hardware-specific library for ST7789
- XPT2046_Touchscreen.h - XPT2046 touch library (Paul Stoffregen)
- SPI.h - SPI communication library
- Wire.h - I2C communication library
- mcp4728.h - DAC MCP4728 library (Benoit Shilling)

All these can be sourced directly throug the IDE's library manager, with the exception of Benoit Schilling's MCP4728 library.

# How to Use
When we first turn the module on, we are welcomed by a black screen divided into two sectors by a vertical line. We are on channel A and we can draw our envelope for that channel.

**Envelope Shapes and Sectors**

Envelopes appear differently from the classic ADSR shapes we are used to. The screen is divided into two sectors with a vertical line. This separates what is equivalent to the attack/decay section from the release section. In other words: everything drawn in the first sector is played as long as there's a gate on signal, everything drawn in the second sector will be outputted when the gate goes out (release).

When sector one finishes, the latest voltage is held (sustain). For the sake of ease of visualization, the sustain level is marked by a violet circle on the vertical line.

**Envelope Offsetting and Stretching**

The first potentiometer sets a DC offset to the drawn envelope. The offset can be positive or negative, but the output is always a positive voltage. This is mainly used to set the zero volt level of the envelope, being not always easy to spot while drawing.

Second and third pots sets the time stretch in each of the two envelope sectors (from half position to right-most position) or the envelope resolution (from half position to left-most). This means that one could have e.g. a fast envelope on sector one, slower on release or viceversa.

Potentiometers and touch screen are, all-in-all, the tools to shape the envelope.

**Change Envelope Channel**

Channels are independet and we can jump from one to the other by pressing the corresponding button. If an EG has already been stored in a channel, it will be loaded and visualized.

**Envelope Triggering**

We can trigger envelopes in two ways: by pressing the channel button and keeping it pressed untill the end of the envelope (it emulates a gate signal) or by sending a gate signal at the corresponding channel input.

**Envelope Copy**

User can copy an envelope from one channel to another simply by keeping pressed the button of the source channel, then pressing the destination channel button.

**LFO mode**

If we draw an envelope inside sector one only, this will be looped "forever". One possible use for this could be the realization of a "name-a-shape-here" low frequency oscillation or even a stepped wave (S&H-like effect) at extremely low resolutions and time stretch.

# Acknowledgments

Many thanks to the nice girls and guys at JLCPCB for sponsoring the manufacturing of PCBs and SMD assembly service for this project. Without their contribution this project would have never seen the light.

JLCPCB is a high-tech manufacturer specialized in the production of high-reliable and cost-effective PCBs. They offer a flexible PCB assembly service with a huge library of more than 350.000 components in stock.
3D printing is part of their portfolio of services so one could create a full finished product, all in one place!

By registering at JLCPCB site via https://jlcpcb.com/IAT (affiliated link) you will receive a series of coupons for your orders. Registering costs nothing, so it could be the right opportunity to give their service a due try ;)

My projects are free and for everybody. You are anyway welcome if you want to donate some change to help me cover components costs and push the development of new projects.
