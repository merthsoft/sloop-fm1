# Latest firmware — 2.4.14 Merthsoft

October 8, 2026. Expands the drum groove bank from seven to sixteen: adds techno,
disco, hip-hop, drum-and-bass, UK garage, reggaeton, bossa, AMEN BREAK and AMEN HALF.
AMEN BREAK is a four-bar, 64-step Amen-inspired kit transcription; AMEN HALF is a
two-bar opening phrase. These use the selected kit, rather than sampled break audio.

On GROOVE, OCT− toggles a looping preview at current tempo without replacing the pattern
or changing transport/recording/undo. OCT+ applies with existing replacement confirmation.
During confirmation OCT− cancels. Preview stops on exit, selection change, apply, STOP,
playback/recording, menu, track changes, panic or project load; drum tails may decay.
The phone discovers all sixteen entries through the existing groove-bank command.

- [Local installer](http://127.0.0.1:8784/webapp/installer/).
- Package: `build/update-2.4.14/firmware/sloop-2.4.14-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `4741238cada50abea20b5c768c6b965c9125ecf281b59b0caa4251194d557c0d`.
- Image 579,656 bytes; static RAM 93,428/98,304; pool 334,560/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Existing ISR budget limitations remain.

Focused bank/preview tests, full UI harness and 48 phone protocol checks pass.
No firmware flash was performed by this chat. See [verification](../../VERIFICATION.md)
and [previous release](../../archive/releases/RELEASE-2.4.13.md).
