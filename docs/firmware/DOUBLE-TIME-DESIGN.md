# DOUBLE TIME punch-in

Implemented in the shared merthsloop workbench on 2026-10-19. No version bump,
commit, push, target build, generated assets, installer or parent-owned DSP changes.

## Behavior

- Former OCT DOWN slot 20 (keyboard key 10, same MIDI effect identity) is now
  DOUBLE TIME. Held internal transport runs at exactly 2x stored BPM. Releasing
  returns to normal, or the still-held HALF gesture, without restarting the song.
- HALF and DOUBLE share the existing rank family: latest held gesture wins within
  the physical source; physical holds take priority over MIDI; otherwise latest
  USB/TRS hold wins. Releasing a winner restores the other held gesture. These
  gestures remain independent of the main effect, filter and gate families.
- The existing one-byte `musical_half` clock snapshot is reused: 0 normal, 1 half,
  2 double. No new clock state/buffer. Exact HALF fractional carry is retained
  across rate changes; DOUBLE requires no carry. Stored BPM never changes.
- Sequencer, arranger, arp (including stopped live arp), count-in, outgoing MIDI
  clock, recording latency, tempo LFO/envelopes, delay/grid times, ducking and
  slicer use the shared rate. PLAY LED compensation follows the effective rate.
  Oscillator/sample pitch, ordinary millisecond envelopes and reverb decay retain
  their existing behavior. Tempo-synced delay changes retain existing crossfades.
- External SYNC bypasses both rate gestures and displays RATE: INT only. Incoming
  USB/TRS MIDI clock remains authoritative, including holdover/disconnection under
  an externally selected source. MIDI clock output retains its existing external
  sync suppression. Doubling an external master would require a separate clock
  multiplication/re-timing policy and could desynchronize other devices.
- The old octave-down granular-delay case/window is removed, reclaiming code.
  Existing ring storage remains shared by the other audio effects.

## VINYL routing handoff

Former MONO slot 25/key 22 is now VINYL (`PX_VINYL`). `punch_vinyl_requested()`
returns selected main-effect status using the same physical/MIDI/local/remote
precedence as the primary renderer. Parent-owned `vinyl.c` calls this helper to
apply the temporary preset before `punch_process`. No mono fold, second ring pass
or wet treatment occurs in `punch_primary` for VINYL. Direct `punch_process` tests
therefore expect unchanged input for VINYL; audible preset/smoothing/full-mixer
coverage belongs to parent-owned vinyl tests. Persistent defaults are unchanged.

## Changed files

- `firmware/src/punch.c`: slot names, rate-family resolution, VINYL request helper,
  remove octave/mono DSP, bypass transport/upstream gestures in primary renderer.
- `firmware/src/core.h`: reuse rate snapshot and musical unit/sample conversions;
  parent's appended globals remain intact.
- `firmware/src/seq.c`, `slicer.c`, `voice.c`: rate snapshot, recording compensation,
  exact slicer boundaries and stopped tempo LFO advancement.
- `firmware/src/ui_layers.c`, `ui_input.c`: status/caption and PLAY LED latency.
- `tests/effects_plan_test.c`, `punch_test.c`, `effects_cpu_test.c`: rate, ownership,
  transparent transport/upstream routing and combined effects checks.

## Validation

Existing WSL Zig 0.13.0 host compiler, explicitly `-UNDEBUG`; binaries are isolated
under `/tmp/sloop-double-*`, no writes to shared build outputs.

- `effects_plan_test`: exact independent phase reference for every BPM 40..240,
  variable blocks and repeated press/release; HALF/DOUBLE fallback and source
  priority; external bypass, STOP cleanup, unchanged BPM; outgoing MIDI pulse
  counts across half/double/normal transitions with no extra START/STOP; exact
  per-track slicer phase through the same transitions. All key pairs, source
  release/reset, 15 main/filter/gate chains, bounded hot input, one ring capture
  and exact dry recovery pass.
- `punch_test`, `scale_test`, `lofi_chord_arp_test`, `slicer_test`: pass with
  assertions enabled, including new DOUBLE/VINYL key/request checks and transparent
  direct audio paths. Existing MIDI routing, arp, recording and grid tests pass.
- `effects_cpu_test`: all eight combined profiles pass, identical one/two-core
  hashes. Worst measured combined profile 501.4 ns/frame (2.304x baseline); host
  timing only, not a target deadline claim. Includes REVERSE + DOUBLE + PHONE +
  GATE and existing vinyl/reverb profiles.
- `effects_plan_test` with `-fsanitize=undefined -fno-sanitize=shift`, assertions
  enabled: passes. Shift exclusion matches existing fixed-point conventions;
  this is not a full strict-C UBSan claim.
- `ui_pages_test`: all pages/layers and 20,000-frame UI fuzz pass with assertions
  enabled.
- Focused `git diff --check` passes. Parent must run final target budget/build and
  package checks against all simultaneous changes; flash savings are unmeasured.

Initial release-mode runs lacked assertions and are excluded from these results.
Synthetic physical holds were corrected to MIDI holds in event-block tests so
normal keyboard layer cleanup does not intentionally release them.
