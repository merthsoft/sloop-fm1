# Latest firmware — 2.5 Merthsoft.1

October 8, 2026. Upstream SLOOP 2.5 (`fa9ce57`) is merged with the Android workstation
and Merthsoft performance/recovery features on branch `android`. Future fork releases
increment `Merthsoft.N`; the leading version identifies the upstream base.

Includes PHYS/NOISE engines, shared engine memory, SYN drum synthesis and 48 kHz USB
capture alongside independent 44.1 kHz USB playback. Retains chord/arp latch gestures,
modifier toggles, sixteen groove starters/preview and full project-load undo/redo.
Protocol 13 preserves companion IDs 72–75 and moves SYN commands to 80–84; the web
editor negotiates routing for upstream protocol 10 and this fork.

Fourteen visualizers remain. Dungeon, Tape, LCD, Bounce, Sloop, Constellation and Lock
Landscape are removed; saved settings for removed styles safely select Oscilloscope.
Original Android work uses the Unlicense with retained [third-party obligations](../../../LICENSING.md).

- [Local installer](http://127.0.0.1:8787/webapp/installer/).
- Package: `build/update-2.5-merthsoft.1/firmware/sloop-2.5-Merthsoft.1.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `6c0de0d42fbf7e639a5db8f5f7cfbcc444beb56eb14c641f441e989a784b3d3d`.
- Image 578,152 bytes; RAM 97,108/98,304; pool 333,948/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass; pool reserve remains intact.
- Android solution: zero warnings/errors; all 25 domain runners pass.
- Firmware audio golden renders (182), voice/routing checks, 10-minute soak, UI fuzz,
  explicit divide-by-zero/undefined-behavior trap checks and USB variants pass.
- Web editor/installer tests pass, including protocol 13 SYN routing and older-firmware compatibility.

The aggregate host runner still reports the existing target ISR static-cost baseline failure;
the baseline was not relaxed. Zig lacks the ASan runtime used by the optional host check;
trap-instrumented UBSan stress ran instead. No new phone installation or physical firmware
flash is claimed for this merge. See [verification](../../VERIFICATION.md) and
[previous release](RELEASE-2.4.16.md).
