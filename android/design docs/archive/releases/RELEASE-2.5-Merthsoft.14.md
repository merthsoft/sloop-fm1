# Latest firmware — 2.5 Merthsoft.14

October 10, 2026. Selected-track modulation locks and illustrated branch guide.

- Hold LFO or ENV, tap HOME, then release to keep vibrato/tremolo and its panel
  locked on the captured synth. Keyboard playing, scale lights and knob edits
  continue. One panel can lock at a time; drums cannot lock.
- HOME unlocks; another function button unlocks on its first press and opens its
  normal page on the next. STOP/panic, menus and selected-track changes clear it.
- Locks are runtime-only. Saved patch parameters remain untouched; no additional
  static RAM, wire or project format changes.
- README and player guide reconciled; a comprehensive illustrated native feature
  guide covers this branch's workflow additions and current visualizers.

Target image **577,872 bytes**; static RAM **97,748/98,304** (556 bytes remaining);
pool **333,948/344,064**; RAM text **925 instructions/no calls**; HAL check passes.
Package **610,066 bytes**, identity **FM-1_900**. SHA-256:
`486fcb221954e7f990837883eea8804d9664aa2a41fb177715235e3c1b5b21f0`.

Focused modulation regression and broad UI/audio regression with 20,000-frame fuzz
pass. Fresh production-code screenshots were visually reviewed. Android code is
unchanged. Physical validation and new ISR deadline/stack measurements are not
claimed; existing ISR cost-budget limitations remain.

See [illustrated feature guide](../../docs/firmware/MERTHSOFT-FEATURES.md),
[performance modulation](../../docs/firmware/MODULATION-WHEEL.md),
[verification](VERIFICATION.md), and
[previous release](archive/releases/RELEASE-2.5-Merthsoft.13.md).
