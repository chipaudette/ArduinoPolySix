ArduinoPolySix
==============

This repository contains code related to my explorations and modifications of my Korg Polysix.  Most of this code is for Arduino and/or Teensy 3.x.

**Arduino Libraries:**  Helper libraries for the other code.

**PolysixDriveController:**  Code driving my custom PCB (Arduino compatible) for enabling different amounts of drive/distortion of one of the OTA stages in my Polysix.

**PolysixKeyAssigner:**  This is the big one.  With this code, plus an Arduino Mega, I've replaced the Key Assigner in my Korg Polysix. When combined with my MIDI keybed (in place of the stock keybed) along with an Adafruit DAC breakout, this software gives me all sorts of new features in my Polysix including portamento, variable detuning between voices, and aftertouch-controlled vibrato.

**PolysixVelocityProcessor**:  When Combined with the PolysixKeyAssigner, plus a Teensy 3.x, plus some wiring, this code for the Teensy enables velocity-sensitivity to control the intensity of the VCF envelope.  Hit the keys harder and the filter opens up more.  Good for getting dynamics into electric piano sounds.

Finally, anything under "Trials" were simply explorations that didn't lead to permanent mods.  This can probably be ignored completely.

PolySixKeyAssigner
==================

To get into "Tuning Mode", press and hold the Arp Button, then switch the "From Tape" switch from LOW to HIGH.
