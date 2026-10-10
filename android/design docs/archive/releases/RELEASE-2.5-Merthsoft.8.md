> Historical release checkpoint. See [current release](../../RELEASE-NOTES.md).

# Firmware — 2.5 Merthsoft.8

October 9, 2026. Adds persistent keyboard scale lights and expands the README's
inventory of implemented additions to upstream SLOOP 2.5.

- Hold physical SEL (SLOOP SCL) and press OCT+ to toggle the dim scale-note guide.
  It follows song ROOT/SCALE; pressed notes stay bright. No octave/latch/pitch change.
  Drum/grid/control layers keep their existing indicators. CHR lights every pitch class.
- Preference persists in the existing settings word, with writes deferred while playing.
  No protocol, project or settings layout change; INFO remains protocol 14.
- All Merthsoft.7 performance, starter library and editing integration is retained.

Target image **574,720 bytes**, static RAM **97,412/98,304** (892 bytes headroom),
pool **333,948/344,064** (10,116 bytes available), RAM text **925 instructions/no calls**.
HAL access check passes. Package **610,066 bytes**, identity **FM-1_900**.
SHA-256: `c4900695f509d2102c9099b8bb569826dd12a4ed862a44dd948f25a925e00cbe`.

Focused production-control regression passes all roots/scales, settings compatibility,
LED priority masks, layer isolation and latch-shortcut cancellation. Existing broad
UI/audio regression and **20,000-frame fuzz** pass. Prior Android/web/domain evidence
belongs to [Merthsoft.7](RELEASE-2.5-Merthsoft.7.md); the Android
app and wire protocol are unchanged by this release. Existing static ISR budget
failures remain unresolved; no new deadline or stack measurements are claimed.
No physical firmware flash or LED brightness validation is claimed.

See [scale-light design](../../../../docs/firmware/KEYBOARD-SCALE-LIGHTS.md),
[player guide](../../../../GUIDE.md) and [release archive](../README.md).
