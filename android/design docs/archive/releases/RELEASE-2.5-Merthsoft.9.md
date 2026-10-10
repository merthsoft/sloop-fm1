> Historical release checkpoint. See [current release](../../RELEASE-NOTES.md).

# Firmware — 2.5 Merthsoft.9

October 9, 2026. Corrects scale-guide selection and keyboard backlight priority.

- Held SEL's key/scale label, tiles and SCALE knob starting value now follow the
  selected synth, matching its tapped SEL page and persistent scale-note LEDs.
  Held root/scale edits still broadcast to all synths; tapped-page edits remain per-track.
- Generic KEYS background lighting is suppressed on synths while the persistent guide
  is active, so off-scale keys stay dark unless played. Disabling the guide restores
  prior backlighting. Drum/grid/control layers retain their lighting.
- SEL + OCT+ still toggles the preference without changing octave, latch or sound.
  CHR intentionally includes all pitch classes. INFO remains protocol 14.

Target image **574,832 bytes**, static RAM **97,412/98,304** (892 bytes headroom),
pool **333,948/344,064** (10,116 bytes available), RAM text **925 instructions/no calls**.
HAL access check passes. Package **610,066 bytes**, identity **FM-1_900**.
SHA-256: `3abc96bbf9ed7ba523b9b373a5de5a2ba8a0fc9302c0d6615b1fbaeda3bb7206`.

Focused production UI/audio regression passes mismatched track scales, selected-track
held display, held SCALE edits, every root/scale and background mode/level, settings
compatibility, layer isolation and latch interaction. Broad UI/audio regression with
**20,000-frame fuzz** passes. Android/protocol code is unchanged. Existing target ISR
cost-budget failures remain unresolved. No physical flash or LED measurement is claimed.

See [scale-light design](../../../../docs/firmware/KEYBOARD-SCALE-LIGHTS.md),
[previous release](RELEASE-2.5-Merthsoft.8.md) and [archive](../README.md).
