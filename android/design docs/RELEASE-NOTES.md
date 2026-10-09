# Latest firmware — 2.5 Merthsoft.2

October 9, 2026. With ARP enabled and CHORD OFF, hold your piano notes, then hold
**ARP for 700 ms** to toggle latch. The current arpeggio keeps playing without a restart;
release the piano keys and it continues. Holding ARP again while notes are physically held
toggles latch off without cutting those notes. SEL supports the same capture gesture.
Generated CHORD-mode latch/modifiers and normal ARP roll controls remain supported.

- [Local installer](http://127.0.0.1:8788/webapp/installer/).
- Package: `build/update-2.5-merthsoft.2/firmware/sloop-2.5-Merthsoft.2.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `79e68ad14ce6e051dd83439b30c9fdd843e599dcefd1f2f71f3126268f32a827`.
- Image 578,248 bytes; RAM 97,108/98,304; pool 333,948/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass.
- Real UI/audio input tests reproduce the old manual-arp capture failure and pass with the fix,
  including existing generated chords, modifier latches, roll controls and 20,000-frame fuzz.
  Divide-by-zero trap instrumentation is enabled. Logs: `build/arp-latch-before.log`,
  `build/arp-latch-after.log`, `build/merthsoft-2-build.log`.

No physical flash/test is claimed for this release. Existing ISR budget/ASan limitations
and upstream merge results remain in [verification](VERIFICATION.md).
See [previous release](archive/releases/RELEASE-2.5-Merthsoft.1.md).
