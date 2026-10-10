# Latest firmware — 2.5 Merthsoft.16

Optimization release; all performance features, instruments, samples and the splash screen remain.

The native sequence browser now keeps key, scale, octave, starter, mode, voice
leading and rhythm choices across track changes until power-off. It seeds these
once on the first visit rather than overwriting them when opening another track.

- Shared visualizer storage saves 1,536 static RAM bytes.
- Shared starter metadata handling and measured UI boundaries retain 800 image bytes of savings after this fix.
- Audio render hashes and tracked DSP/ISR costs match the preceding build.
- Regression checks cover shared histories, pitch mapping, undo and stress; test-oracle and stale-sanitizer runner issues were corrected.

Target image **577,072 bytes**; static RAM **96,212/98,304** (2,092 free); pool **333,948/344,064**. HAL and call-free 925-instruction RAM code checks pass.

Package **610,066 bytes**, identity **FM-1_900**. SHA-256:
`05d8075ed7dc8cecb0dcd1c9e75bcc69af570e5600f2c8b4711fbdffca61d658`.

Local installer: http://127.0.0.1:8802/webapp/installer/

Starter UI, phone protocol and broad UI regressions pass. Existing LOFI golden and historical ISR-budget failures reproduce on the untouched baseline. Physical device validation is not claimed. See [optimization evidence](../../docs/firmware/OPTIMIZATION.md), [verification](VERIFICATION.md), and [previous release](archive/releases/RELEASE-2.5-Merthsoft.15.md).
