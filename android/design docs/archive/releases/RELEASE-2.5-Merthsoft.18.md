# Latest firmware — 2.5 Merthsoft.18

- Held LFO/ENV replace the note grids with one-cycle waveform displays, a live audio-phase marker and current vibrato cents / tremolo level percentage.
- Track/key context and a compact, correctly spelled scale guide remain visible. Knobs and physical/MIDI input mapping are unchanged.
- Retains whole-pattern EDIT octave, shift/pitch readouts, cross-track starter settings and firmware optimizations.

Target image **578,912 bytes**; static RAM **96,244/98,304** (2,060 free); pool **333,948/344,064**. HAL and call-free 925-instruction RAM code checks pass.
Package **610,066 bytes**, identity **FM-1_900**. SHA-256: `2702320a98c7434850b8831fc8e7a21ab04a2898001bbadb5cb55bf4f8a5608b`.
Local installer: http://127.0.0.1:8804/webapp/installer/

Display bounds/zero-depth and non-mutating draw checks, modulation regressions and broad UI regressions pass. A new project/NOR test overwrites B, clears RAM, reloads and verifies the replacement notes and instrument settings. This has not reproduced the reported hardware save loss; its cause remains unresolved pending device evidence. No save-path fix is claimed.

See [modulation panels](../../docs/firmware/MODULATION-WHEEL.md), [verification](VERIFICATION.md), and [previous release](archive/releases/RELEASE-2.5-Merthsoft.17.md).
