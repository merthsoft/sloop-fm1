# Latest firmware — 2.5 Merthsoft.10

October 9, 2026. Live modulation wheel on hardware, MIDI and Android Perform.

- Hold LFO + turn knob 1 for transient 0–127 modulation; release clears physical
  ownership and restores the latest MIDI wheel value. Saved LFO rate/depth stays intact.
- MIDI CC1 follows synth-channel routing; physical control takes priority while held.
  Wheel adds about half a semitone at maximum, using existing rate/wave/fade without
  retriggering held, latched or arpeggiated notes. Drums and IN CLOCK ignore CC1.
- STOP/panic/reset clear modulation. USB reset/detach clears USB-owned values;
  TRS-owned values remain. Patch/project formats and INFO protocol 14 are unchanged.
- Android Perform adds a momentary horizontal CC1 strip with release-to-zero,
  pointer ownership, coalescing, lifecycle cleanup and XY-macro exclusion.
- Retains Merthsoft.9 selected-track scale-guide and keyboard-backlight corrections.

Target image **575,360 bytes**, static RAM **97,444/98,304** (860 bytes headroom),
pool **333,948/344,064** (10,116 bytes available), RAM text **925 instructions/no calls**.
HAL access check passes. Package **610,066 bytes**, identity **FM-1_900**.
SHA-256: `38609157694869baad071fc4050a0185fbb15da8a2972089bb731fc933ccfd28`.

Focused modulation and scale-guide production UI/audio regressions pass. Broad UI/audio
regression and 20,000-frame fuzz pass. Performance-controls domain runner passes
**3,035 checks**. Android APK build passes with **zero warnings/errors**. Physical
firmware/phone gesture testing is not claimed; no device was flashed or app installed.
Existing target ISR cost-budget failures remain unresolved; no new deadline measurement
is claimed. TRS unplug detection, controller CC1 forwarding through phone input and
modulation gesture recording are outside this slice.

See [mod-wheel contract](../../../../docs/firmware/MODULATION-WHEEL.md),
[player guide](../../../../GUIDE.md), [previous release](RELEASE-2.5-Merthsoft.9.md)
and [release archive](../README.md).
