# Latest firmware — 2.4.16 Merthsoft

October 8, 2026. Successful saved-project loads, stopped section selection and restored
working-project backups preserve a complete pre-load snapshot. While stopped, hold EDIT
and press OCT− to undo the load; OCT+ redoes it. Normal sequence edits and recording take
over undo history. Empty/failed loads preserve history. Restart clears the RAM snapshot.
This protects future loads and cannot reconstruct music overwritten on older firmware.

Android Perform now re-attacks repeated latched chord-pad presses, with Voice lead beside
Latch. Its build passes and the updated APK is installed on the Pixel; 26 Transport checks
cover note-off/on order, bass, modifier common tones and shared-note cleanup.

- [Local installer](http://127.0.0.1:8786/webapp/installer/).
- Package: `build/update-2.4.16/firmware/sloop-2.4.16-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `194ea161909daf77cb3c70a65138c97c4623ac03275aa2c5fa1e1d5f7946b596`.
- Image 581,144 bytes; RAM 97,300/98,304; pool 334,560/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Pool reserve remains intact.
- Project recovery, UI input/render/fuzz, sequencer lifecycle and song/audio suites pass.
- Retains sequencer-fed arps, sixteen groove starters and non-destructive hardware preview.

No firmware flash was performed by this chat. The physical recovery gesture awaits testing
on the FM1. See [verification](../../VERIFICATION.md) and [previous release](RELEASE-2.4.15.md).
