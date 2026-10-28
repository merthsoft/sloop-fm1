# Master LOFI processing optimization

This is a performance change to the shared master LOFI processor, not an
instrument change. It preserves the .88 effect samples and complete internal
state across control, mode, punch and transport changes. The motivating hardware
workload is drums plus Flute Dust/GRAIN chords: enabling any master LOFI type
caused the 95% wall-time guard to shed voices. The LOFI instrument engine did not
reproduce it. The guard, voice budget, grain density and samples are unchanged.

## Work removed

- Detect settled controls once per block. The sample loop skips the seven
  unchanged slews; coefficient and CD-window calculations run once per block.
  Knob, punch, mode and transport changes retain exact per-sample updates.
- CD and SAMPLER no longer calculate unused fractional-delay interpolation or
  noise output. RNG, noise/damage histories, phase and ring writes still advance
  exactly, preserving subsequent transitions.
- WOW OFF and settled CD/SAMPLER skip unused sine/delay endpoint calculations.
  Phase still advances normally, including while the delay is unused.
- Shared stereo controls are held locally for the frame. Filter histories remain
  separate, with no stereo crossfeed or change to channel order.
- Replace bounded division by127 with an exact reciprocal multiply for the
  proven 0..32767 domain. Replace signed power-of-two divisions with explicit
  truncation toward zero, preserving negative rounding. Eight static hardware
  divide sites in the .88 sample loops become zero. The new target guard also
  requires zero helper calls in those loops. Control setup may still divide.

## Evidence and limits

Frozen .88 code is retained privately under build/lofi-deep for same-process
reference comparisons. The committed lofi_fastpath_test stores frozen output
and full-state hashes for seven modes, four long scenarios each: stable controls,
WOW OFF, fade to/from bypass, and changing controls/modes/punch/transport with
varying block lengths and independent stereo bus input up to ±4 million. It also
checks all 32,768 inputs in the reciprocal domain and signed-shift boundary cases.
UBSan and the older direct 14,000-block reference/state comparison pass.

Focused character, pitch/noise, sampler headroom, STOP/bypass/punch and Flute Dust
chord-gate tests pass. The STOP test previously expected SAMPLER to generate
independent noise. Frozen .88 reproduces that assertion failure; both SAMPLER
and CD correctly produce silence without input. The revised test explicitly
asserts zero, while the noise-generating modes must still produce output.

Native host benchmarks use alternating order, three trials and median process
CPU time, with identical steady inputs, knobs and 2,048 warmup blocks. These are
host measurements, not FM-1 cycle measurements. Target disassembly independently
confirms the removed divides and outlined/inlined structure. The nested-span
cost estimator overcounts overlapping conditional slow-path spans here; its
aggregate score is not a valid estimate of the selected steady path.

The actual FM-1 deadline improvement remains unverified. The diagnostic .89
build retains STEALS/SHEDS/MAX US/LATE/C1MASK in About for a matching device run.

## Space and rejected trials

Same diagnostic configuration: .88 is 583,668 B; .89 is 583,892 B, a 224 B code cost.
Normal .89 also builds:584292/584336B,44Bfree. No static RAM growth: normal main
97,324 B; low 28,928 B; pool 335,424 B. Diagnostic main is 97,340 B due to existing counters.
No feature, About credit or sound content was removed to fit these changes.

Alias qualifiers produced identical target code and were discarded. An extra
branch skipping inactive punch boosts matched the frozen sound but made the
normal build overflow by 84 B, so it was rejected. The slower and steady paths
share one renderer rather than duplicating a full loop for every character.

Reproduction logs, snapshots and benchmarks are in ignored build/lofi-deep;
normal89-verified.log and build89.log contain the two actual final target links.
