# Latest firmware — 2.4.10 Merthsoft

October 8, 2026. Fixes the 2.4.9 shortcut rejecting chords while ARP was enabled. Works with ARP on or off. While holding a physical chord, hold ARP or SEL (SLOOP label: SCL)
for 700 ms to toggle latch once. Enabling latch preserves the sounding notes when you
release the keys; with ARP on, its note pool remains latched and the arpeggio continues. Disabling latch leaves held keys sounding until their release.
Using a layer knob, playing another key, changing track, holding another layer or HOME,
or releasing the chord cancels the shortcut. Without an existing held chord, normal
layer behavior is unchanged. Modifier toggles from 2.4.8 remain supported.

- Package: `build/update-2.4.10/firmware/sloop-2.4.10-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `981e68bda9a6d4331e6e884f024bd610f8a2cd67f8b57563efed6062ab153c1d`.
- [Local installer](http://127.0.0.1:8780/webapp/installer/).
- Target image 571,820 bytes; static RAM 92,964/98,304; pool 334,560/344,064;
  RAM-text 925 instructions/no calls; HAL register checks pass.
- Real UI/audio harness passes, including held-chord voice gates, MIDI event preservation,
  once-per-hold, cancellation, release behavior and 20,000 random frames.
- Scale/keyboard/modifier latch regressions pass. No hardware flash performed by this chat.

Android integration evidence is in [VERIFICATION.md](../../VERIFICATION.md).
Earlier releases are retained in the [archive](../README.md).