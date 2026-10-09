# Latest firmware — 2.5 Merthsoft.3

October 9, 2026. The 16 punch-in effects now have six categories of black-key controls
while FX is held or its layer is locked. White-key effects retain their original IDs.

| Physical key | Control |
| --- | --- |
| F♯3 / G♯3 | Slower / Faster |
| A♯3 | Triplet |
| C♯4 or C♯5 / D♯4 or D♯5 | Gentle / Extreme |
| F♯4 or F♯5 | Blend |
| G♯4 | Latch toggle |
| A♯4 | Retrigger |

Most controls are momentary; latch survives releasing the white key and FX. Hold FX and
press G♯4 again to unlatch; STOP clears latch. Retrigger fades to dry, recaptures/restarts
using the effect's normal transport alignment, then fades back. Opposing rate/strength
keys cancel. Repeated modifier keys remain effective until all copies are released.
Tiles show the adjacent black-key mapping, and the header/LEDs show latch state.
See the [guide](../../GUIDE.md#black-key-punch-modifiers-25-merthsoft3) and
[component design](../../firmware/PUNCH-FX-DESIGN.md) for interaction and DSP details.

- [Local installer](http://127.0.0.1:8789/webapp/installer/).
- Package: `build/update-2.5-merthsoft.3/firmware/sloop-2.5-Merthsoft.3.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `9be5c106a93633cca50e1ebd2248541b67739f70ad46350a1db393f39c89ebea`.
- Image 579,492 bytes; RAM 97,204/98,304; pool 333,948/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Splash retained.
- 160 effect/modifier combinations pass bounded audio, audible-control and exact dry-cleanup checks.
- Real keyboard/UI tests cover latch/retrigger/STOP, duplicated modifier ownership, no note
  playback/recording and 20,000-frame fuzz; undefined-behavior trap instrumentation passes.
- Physical/remote FX priority tests pass. 182 existing golden audio renders are unchanged;
  voice/routing, health and host CPU checks pass.

No physical firmware flash is claimed. The existing ISR static-budget and unavailable ASan
runtime limitations remain recorded in [verification](VERIFICATION.md).
See [previous release](archive/releases/RELEASE-2.5-Merthsoft.2.md).
