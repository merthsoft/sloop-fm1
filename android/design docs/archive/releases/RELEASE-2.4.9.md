# Latest firmware — 2.4.9 Merthsoft

October 8, 2026. While holding a physical chord, hold ARP or SEL (SLOOP label: SCL)
for 700 ms to toggle latch once. Enabling latch preserves the sounding notes when you
release the keys. Disabling latch leaves held keys sounding until their release.
Using a layer knob, playing another key, changing track, holding another layer or HOME,
or releasing the chord cancels the shortcut. Without an existing held chord, normal
layer behavior is unchanged. Modifier toggles from 2.4.8 remain supported.

- Package: `build/update-2.4.9/firmware/sloop-2.4.9-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `35ea33534c8bf3c036466ffe8d15402103d8633325aaed0fdc1542362fda9605`.
- [Local installer](http://127.0.0.1:8779/webapp/installer/).
- Target image 571,836 bytes; static RAM 92,964/98,304; pool 334,560/344,064;
  RAM-text 925 instructions/no calls; HAL register checks pass.
- Real UI/audio harness passes, including held-chord voice gates, MIDI event preservation,
  once-per-hold, cancellation, release behavior and 20,000 random frames.
- Scale/keyboard/modifier latch regressions pass. No hardware flash performed by this chat.

Android integration evidence is in [VERIFICATION.md](../../VERIFICATION.md).
Earlier releases are retained in the [archive](../README.md).