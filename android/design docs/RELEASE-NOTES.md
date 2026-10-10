# Latest firmware — 2.5 Merthsoft.11

October 9, 2026. Selected-track performance vibrato and tremolo.

- Hold LFO for vibrato or ENV for tremolo; release restores the previous screen.
- Knobs 1–4 control speed, depth, waveform and fade-in independently of the patch.
- Only the selected synth captured on press is affected. Keyboard, latch and arp
  continue normally. Quick taps retain normal LFO/ENV page navigation.
- Retains MIDI/Android CC1, source cleanup, scale lights and previous features.
- Runtime settings persist between holds until reboot; drums are unaffected.

Target image **576,056 bytes**; static RAM **97,508/98,304** (796 bytes headroom);
pool **333,948/344,064**; RAM text **925 instructions/no calls**. HAL check passes.
Package **610,066 bytes**, identity **FM-1_900**. SHA-256: `74ca04df2d3fb52d827547f753b6ab794807eb474d3de18ccd79193193f97969`.

Focused native modulation tests and the broad UI/audio suite with 20,000-frame fuzz pass. Hardware audible/gesture testing is pending;
no device was flashed. Android implementation is unchanged. Existing target ISR
cost-budget failures remain unresolved; no new deadline measurement is claimed.

See [performance modulation](../../docs/firmware/MODULATION-WHEEL.md),
[player guide](../../GUIDE.md) and [previous release](archive/releases/RELEASE-2.5-Merthsoft.10.md).
