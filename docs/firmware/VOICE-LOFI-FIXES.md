# Held-note recovery and render scheduling

The [investigation](VOICE-LOFI-FINDINGS.md) isolated two software defects that
can make master LOFI expose missing chord tones: overly broad VOICE worker
exclusion and irreversible CPU shedding of ordinary held notes. It did not
establish a deterministic fault in the LOFI signal processor or measure actual
FM-1 engine costs.

## Safety contracts

- Worker eligibility must use current rendering activity after events, including
  release tails, old-engine fades and newly adopted engines. An empty configured
  VOICE track must not exclude a sounding VOICE track. Concurrent VOICE rendering
  must still protect its shared breath-noise generator.
- Automatic scheduling may profile real render units, but may publish profiles
  only after worker completion. Source changes, new activity and timeout fallback
  must not reuse misleading or partial measurements. Manual masks retain safety
  filtering.
- Recovery applies only to notes deliberately removed by CPU protection that
  still have a logical owner. Voice-limit replacement and natural sample endings
  must not become repeated attacks.
- Recovery requires an inactive slot, admission capacity and sustained full-wall
  deadline headroom. It must not steal another note, bypass engine caps or disable
  overload protection. Repeated overload must back off rather than oscillate.
- Releases, source changes and all-notes-off cancel pending recovery. Existing
  drone recovery remains distinct from ordinary held-note recovery.

## Implemented policy

Current active voices, including release tails and fading old engines, determine
the VOICE shared-RNG exclusion. Engine adoption and pending onsets occur before
this check. Automatic scheduling measures complete synth/drum units with the
hardware timer, scales profiles for voice-count changes and compares at most
16 legal masks. A previous split within 1/32 of the optimum remains selected.
Unmeasured active units use the original estimate until profiled; engine,
preset, source and fade changes invalidate matching profiles. Manual masks and
worker timeout fallback retain their safety filtering.

CPU recovery reuses the existing lost-drone-pitch map, distinguishing CPU
markers and original velocity from native drone markers. It requires at least
half a second continuously at or below 75% of the full DMA-half wall-time budget.
Each restoration consumes that headroom history; successive attempts wait one,
two and four seconds. Eight seconds of uninterrupted headroom resets the backoff.
DMA lateness resets the headroom history. The displayed CPU average is not used.

Readmission is deliberately conservative: POLY synths with nonzero sustain,
ordinary envelope lifetime and a current physical-key, MIDI, sequencer,
generated-output or latch owner. SAMPLE/SLICE, PHYS, engine amplitude callbacks
and engine-owned envelopes are excluded from ordinary CPU recovery. GRAIN
source and sample-generation changes cancel its pending pitches. No naturally
finished voice is queued. Native drone recovery retains its existing behavior.
Restoration starts a new local voice attack; it cannot reconstruct the oscillator
or envelope position of the faded voice. It sends no new MIDI note-on.

## Qualification and size

The combined normal **2.8.0.93** image is **584,304 / 584,336 B**, leaving
**32 B flash**. Main static RAM is **97,548 / 98,304 B** (756 B free), low RAM
**28,928 / 29,392 B**, and pool **335,424 / 344,064 B**, retaining its 8,192 B
reserve. Compared with the actual .92 baseline, this costs 1,980 B flash and
48 B main RAM. No sample packing, effect removal or higher voice cap funds it.

The focused scheduler and actual-ISR recovery tests cover active/fading/adopted
VOICE eligibility, measured legal partitions, stale profiles, timer wrap,
worker timeout, ownership, releases, source changes, natural sample endings,
voice-limit replacement and recovery backoff. Chaotic threaded execution and
UBSan checks pass. The broader integration suite passes 44 programs, including
full split/timeout checks; all 200 audio golden renders remain unchanged.

Target loop/inlining and LOFI divide/call guards pass without raising any
budget or call cap. Their backward-branch parser now rejects noncyclic cold
rejoins, covered by six regressions for real/nested loops, early returns,
predication and opaque control flow. With the same corrected estimator, IRQ
loop cost is 134 with two calls versus the .92 baseline's 137 with two calls;
this is an instruction-structure comparison, not measured execution time.
Literal restoration checks, native/WASM audio parity and all 16 Pages deployment
tests pass. The local installer package is 610,001 B; SHA-256:
`5f6b2899940cdc4bb13f2042eed72185a898e87d167c5e640f8108b83ab98f4e`.

Injected-clock tests demonstrate policy behavior, not target execution time.
Hardware deadline and audible dropout qualification remain necessary. Private
builds, rejected layout trials and logs are under `build/dropout-fix-93/`.
