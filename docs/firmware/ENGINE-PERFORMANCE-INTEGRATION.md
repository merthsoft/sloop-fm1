# Sound engine performance integration

Ten measured, exact-output candidates from the four engine audits are integrated
in .90. The existing master LOFI optimization is retained. Voice limits, the
expanded admission setting, the 95% two-half overload guard, factory audio,
project formats and musical behavior are unchanged. Device deadline improvement
still needs a matching hardware run; host equivalence is not a timing guarantee.

## Integrated changes

| Engine | Change | Contract retained |
| --- | --- | --- |
| GRAIN | Calculate density interval only when a grain is due | Same countdown, parameter sampling, RNG and spawn timing |
| GRAIN | Align reverse decode windows except at the two-sample interpolation boundary | Same samples, pitch, grain lifetime and source readiness |
| SAMPLE/GRAIN/drums | Share the exact inline IMA magnitude calculation | Same rounding, predictor/index clamps, loop and held-tail policy |
| SAMPLE/GRAIN pitch setup | Specialize the known pitch-table range | Same table values and full defined-domain integer results |
| ANALOG | Skip the second-increment calculation when zero detune overwrites it | Oscillator phases still advance |
| DIGITAL | Precompute ratio saturation limits from the same compile-time ratio list | Same increments and Nyquist limit for every uint32 input |
| LOFI instrument | Precompute immutable WRAM means | Signed division rounding and all table/chip choices preserved |
| FM6 | Look up carrier masks instead of scanning six operators repeatedly | Exact FM6_ALG bus/output interpretation, including algorithm aliases |
| DRAWBAR | Cache unchanged registration, percussion, click and drive setup | Rotor phases/speed/fades stay live; all seven raw control keys checked |
| PHYS MODAL/MEMB | Use existing ordered native clamp at six missed sites | Same signed bounds and single evaluation; other PHYS paths unchanged |

The DRAWBAR cache uses 16 logical bytes inside the already allocated per-engine
arena. Static assertions retain the existing PHYS-sized maximum for DRAWBAR,
GRAIN and FM6. DIGITAL's ratio values and limits have one macro-defined source;
WRAM means and FM6 masks have maintained derivation checks. The reverse-window
edge fallback is retained deliberately: at an exact 128-sample edge, spawn
requires both hi-1 and hi. Blind alignment would lose the former sample.

## Combined target measurements

Actual pinned JieLi Clang 4.0.1 -Oz links, unchanged slot and RAM bounds:

| Configuration | Image B | Flash free B | Main static B | Low RAM B | Pool B |
| --- | ---: | ---: | ---: | ---: | ---: |
| Frozen .89 normal baseline | 584,292 | 44 | 97,324 | 28,928 | 335,424 |
| Combined .89 normal experiment | 584,228 | 108 | 97,324 | 28,928 | 335,424 |
| Final .90 normal | 584,228 | 108 | 97,324 | 28,928 | 335,424 |
| Final .90 diagnostic | 583,844 | 492 | 97,340 | 28,928 | 335,424 |

The same-version combined result saves 64 bytes net, including the DRAWBAR cache
and reverse-window speed/flash tradeoffs. Independent audit deltas were not
summed. Diagnostic About uses different code/strings and its smaller size does
not establish the normal-build margin. No static RAM grew in either matching
configuration. Pool gap still includes the existing 8,192-byte reserve, with
448 bytes beyond it. These are static extents, not stack-watermark results.

In the audit's copied Flute Dust/drums scene, lazy density evaluates the interval
48 rather than 4,500 times. The independently tested aligned reverse candidate
reduces GRAIN decode calls by 4.15% in that scene. These operation counts are
neither total CPU percentages nor an additive prediction for the combined build.
The pitch specialization also changes the remaining helper's division count.

## Validation

The integrated source passes:

- 200 unchanged audio goldens, voice/routing/health checks and UI navigation/fuzz.
- Twelve real mixer scenes across thirteen core masks, chaotic two-core execution
  and lost-worker fallback. The new flute_source scene preserves the frozen .89
  audio hash f6e48f0462161b93 through simulated user-slot replacement, generation
  invalidation, emptying and return to built-in flute. It tests live metadata
  invalidation, not concurrent physical flash programming.
- 328,458 reverse grains and 44,422,594 actual production render calls compared
  with the original full-window algorithm under UBSan, including cold/full
  indexes, reused source storage, all offsets, six rates and variable block sizes.
- All 251,658,240 DIGITAL upper-increment/ratio combinations, all defined pitch
  inputs -3072..6143, all carrier masks/aliases and signed WRAM means.
- 275,400 sample-decoder boundary/full-state comparisons, including loop/held
  behavior and the defensive index cap; exhaustive shared IMA magnitude checks.
- 200,000 DRAWBAR blocks against the uncached control path, including rotor-only
  changes on cache hits and repeated arena invalidation. Its original audible
  control and rotor state match exactly.
- Frozen original ANALOG output/full-state comparison over 20,000 changing blocks
  and DIGITAL/LOFI comparisons over 40,000 changing blocks.
- FM6 algorithms/envelopes/macros/AMS/formats/bank checks, shared-arena adoption,
  user kits, engine ownership and 20,726 actual target-clamp register cases.
- Target placement, existing DSP budgets/inlining, zero-division/call master LOFI
  loop checks, and byte/address-exact boot literal restoration under UBSan.

New maintained tests are engine_control_tables_test.c, sample_decoder_test.c,
drawbar_cache_test.c and grain_reverse_test.c. GRAIN_REVERSE_FULL=1 selects the
full reverse matrix; the default concentrates on all boundary classes. The
original private full-render oracles and target evidence remain in ignored
build/engine-integration. No golden output was updated to accept a difference.

## Deferred findings

PHASE duplicate setup, blanket PHYS clamp replacement and the string-delay hoist
were not recommended gains under the measured code/stack tradeoffs. The larger
PHYS coefficient cache, FORMANT setup cache, FM6 shared AMS work and exact TRIO
reciprocal ideas are unimplemented investigations, not integration-ready patches.
They remain documented in their audit reports. No approximate DSP, quality
reduction, feature removal, new PCM cache or runtime sample compression was added.

Audit sources: [SAMPLE/GRAIN/drums](ENGINE-SAMPLE-PERFORMANCE-AUDIT.md),
[oscillators](ENGINE-OSCILLATOR-PERFORMANCE-AUDIT.md),
[FM6/TRIO/DRAWBAR](ENGINE-FM-PERFORMANCE-AUDIT.md),
[VOICE/PHYS](ENGINE-VOICE-PHYS-PERFORMANCE-AUDIT.md).
