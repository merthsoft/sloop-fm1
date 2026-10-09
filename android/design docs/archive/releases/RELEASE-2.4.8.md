# Latest firmware — 2.4.8 Merthsoft

October 8, 2026. Physical chord modifier keys now toggle while chord latch is enabled.
Release preserves the quality, another press removes it, and new roots inherit the toggles.
Modifiers remain momentary with latch disabled. CHROM, latch disable, arp mode, STOP and
panic clear toggles; each synth owns its own qualities.

- Package: `build/update-2.4.8/firmware/sloop-2.4.8-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `80b7284f39d8130efcf0990063b36a53249f9996d3c3545163299804939e0c53`.
- Installer: `http://127.0.0.1:8778/webapp/installer/`.
- Target: image 571,500 bytes; static RAM 92,948/98,304; pool 334,560/344,064;
  RAM-text 925 instructions/no calls; HAL register checks pass.
- Scale/keyboard latch regressions and sequencer suite pass. No hardware flash performed.

Android follow-up integrates draft audition/saving and progression/rhythm controls,
retained sample browsing/tap chopping, and multi-note piano-roll editing. Build/test
evidence and deployment scope are in [VERIFICATION.md](../../VERIFICATION.md).
Earlier releases are retained in the [archive](../README.md).
