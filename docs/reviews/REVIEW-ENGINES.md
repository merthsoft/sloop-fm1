# Engine and voice review — current working tree

Reviewed 2026-10-23, HEAD `9fd9ddc2f45d57ed0647270d757c2ead7ba30a0a` plus the live uncommitted tree. This is a read-only production review; only this report and `build/code-review-engines-51/` were written. Concurrent reviewers/experiments may change the tree after inspection. No firmware build, shared generated output, Git ref, or shared summary was changed. The requested 581296/584336-byte target was supplied context, not independently rebuilt here.

## Integration recommendation

Address the two confirmed P2 behavior defects below in separate narrowly scoped changes. Keep current arena ownership, bounded grain decoding, engine ordering, factory content, and CPU1 exclusions. Investigate the TRIO simplification only with target measurements and exact-output regression. No measured flash saving is claimed by this review, and no production patch is included.

## Ranked confirmed defects

### 1. P2 — MIDI note zero is confused with an empty mono stack

References: `firmware/src/voice.c:530`, `firmware/src/voice.c:573`, `firmware/src/voice.c:575`; `mono_pick` at lines 464–479.

Trigger: LEGATO, hold note 0, press note 60, then release note 60. The stack still contains note 0, but `if (next)` treats the valid fallback as no held note. The lead releases and note 0 remains held but inaudible. MONO and UNISON use the same fallback. In addition, `t->mono_note && ...` treats a sounding note 0 as no sounding note, causing LEGATO/UNISON to retrigger when moving from it.

Private production-source reproduction output:

```
mono-zero: held=1 stack0=0 voice_note=60 gate=0 (expected note0 gate1)
```

Minimal fix: determine stack emptiness from `t->nmono` after removal, rather than the pitch returned by `mono_pick`. Determine sounding state from the lead's active/gate state plus explicit held-stack state, without reserving pitch 0. Verify mode-change semantics when choosing that predicate; simply changing one conditional misses the retrigger defect.

Why current tests miss it: `tests/regress.c:537` uses notes 36–83; MIDI ownership stress at line 710 uses 36–71, and protected mono coverage holds note 40. Arena random notes are 36–83 (`tests/arena_test.c:216`). Golden phrases do not establish the note-zero fallback invariant.

Meaningful regression: for MONO/LEGATO/UNISON and LAST/LOW/HIGH priority, exercise 0→60→release60 with 0 still down, then release0 and verify all gates clear. For LEGATO/UNISON additionally capture voice age, envelope and pitch-envelope state when changing from note0; confirm no unintended retrigger. Include public MIDI routing coverage as well as direct allocator coverage.

### 2. P2 — several pitch consumers discard the caller's fine-pitch factor

References: `firmware/src/voice.c:886`–889 constructs `m.fine` and adjusts `m.inc`; its contract is documented in `firmware/src/core.h:161`. Consumers rebuilding pitch omit this residual:

- `firmware/src/eng_phase.c:140`–141: line 2 uses integer `m.pitch16` plus DTN residual, omitting `m.fine`, even with DTN=0.
- `firmware/src/eng_analog.c:49`–50: oscillator 2 omits it with nonzero DTN. The DTN=0 special case at line 58 correctly restores `m.inc`.
- `firmware/src/eng_lofi.c:156`: rebuilding after sweep/arp discards it entirely.
- `firmware/src/eng_sample.c:176`–177: sample playback rate uses integer pitch only.

Consequences: UNISON detune does not spread LOFI/SAMPLE playback rates; fine global tuning and fractional glide/bend/modulation become quantized there. PHASE's two equal-pitch lines drift relative to one another under a common fine adjustment; ANALOG changes the common tuning of oscillator 2 when DTN leaves zero. These are pitch contract failures, not requests to remove intentional chip quantization or factory detune.

Private direct-render reproductions use pitch16=960, a fine factor of +100/4096 (reachable within supported unison range), and CTL=32:

```
phase fine100: osc1advance=835267776 osc2advance=815363776 (DTN0 expected equal)
lofi: fine100 advance=815363776 fine0 advance=815363776
sample: fine100 pos/frac=21/8192 fine0 pos/frac=21/8192
```

The fixture uses production functions and a consistent `vmod_t`, isolating frequency progression from effects or amplitude. The numerical +100 factor is an allowed fine-factor test value, not a claim of +100 cents.

Related source-only observations: `eng_grain.c:234` also omits the factor when spawning grains; `eng_noise.c:153`/161 rebuild LFSR/META clock pitch without it when their offset/drift path is used. Those paths were inspected but not separately reproduced. `eng_phys.c:161` intentionally computes sympathetic chord pitches separately; do not automatically change fixed ROOT sympathetic tuning. Optional SLICE also rebuilds pitch, but is excluded from the default target and needs its varispeed semantics considered separately.

Minimal fix: include `m.fine` in rebuilt oscillator increments, and apply the corresponding Q12 rate adjustment to sample/grain steps with wide or proven-bounded arithmetic. Preserve the existing decode-rate caps, rounding where fine=0, noise clock saturation, chip sweep/arp behavior, and fixed sympathetic ROOT semantics. Avoid a blanket generic frequency framework. Treat this as a deliberate correction of affected sound, with reviewed golden changes where needed.

Why current tests miss it: hashes preserve existing sound but do not assert the relationship between common tuning and each oscillator/playback cursor. Typical default tune=0 and fixed-note phrases omit sub-1/16-semitone movement. A golden can faithfully retain this defect. UNISON health/cap checks do not assert frequency spread.

Meaningful regression: use identical initial states and common fine factors of both signs. Check phase/cursor advancement for every oscillator and sample rate consumer, including DTN zero/nonzero, LOFI sweep/arp, and noise offsets. Check SAMPLE unison cursor separation and smooth fractional bend/glide. Keep fine=0 exact-output comparisons and source-decode cap assertions. For GRAIN inspect a newly spawned grain's step with identical RNG/source state.

## Exact-output optimization opportunity — not target measured

`firmware/src/eng_trio.c:276`–280 redoes `pitch_inc(m->pitch16)` and divides the difference of `m->inc` and that base to reconstruct the factor already held in `m->fine`. The production caller (`voice.c:886`–889) establishes exactly that relationship. A minimal candidate is `int32_t fine = m->fine;`, removing the reconstruction and leaving `trio_inc` unchanged.

Private exhaustive arithmetic validation over all 2048 pitch indices and factors −160…180 (698,368 inputs) produced **zero mismatches**. This range covers the current unison/tuning/fraction residual range with margin. This demonstrates the arithmetic equivalence for those inputs, not complete render equivalence or a target cycle/byte delta. Review all callers before adopting, because manually constructed test `vmod_t` values must satisfy the contract too.

Regression/measurement required: exact TRIO output at all factory presets plus nonzero fine, offsets, ring and sync; target JieLi -Oz object/link comparison and timing/call inspection. The benefit is one avoided reconstruction/division per TRIO voice/block in source. Compiler elimination and actual instruction/flash savings remain unmeasured. No savings from already integrated pitch-table compression, inline policies, or preset storage are counted again.

## Coverage map and areas with no new confirmed defect

| Area | Inspection performed | Result / limit |
| --- | --- | --- |
| `voice.c` | Allocation, global budget/victims, shedding, retrigger state preservation, mono stack/priorities, note on/off, strum cancellation, engine/preset fades, envelope/glide/pitch/control render | Confirmed note-zero defect and fine-factor caller contract. Age wrap and every live mode-change combination were not exhaustively tested. |
| `engines.c` | Include/table order, shared arena takeover clearing, PHYS RNG prediction, preset trim lookup | Ownership design is explicit; table indices have static assertions. No removal recommendation for active engines. |
| ANALOG / DIGITAL / PHASE | Oscillator increments, mix/filter arithmetic structure, ratio saturation, phase preservation, PD setup/wave/render | Fine omission in ANALOG/PHASE; DIGITAL derives ratios from corrected `m.inc`. No full arithmetic-range proof. |
| LOFI / SAMPLE | Sweep/arp frequency setup, playback cursor/interpolation, ADPCM decoder bounds, zone validation and note selection | Fine omission reproduced. ADPCM source/loop policies intentionally differ from grain/slice; no deduplication proposal. Live upload/erase transaction lifetime was not audited end to end. |
| GRAIN | Index ownership/reset/build/seek, source changes, grain spawn/load cap, reverse refill, window interpolation, per-voice grain cleanup | Explicit caps and per-part state retained. No new bounds defect confirmed; new-grain fine omission is source-only extension of finding 2. |
| FORMANT | Vowel interpolation, ratio/coefficient generation, note/render state, shared noise and resonators | Shared noise ownership checked against CPU1 scheduling; no new reproduced defect. Fixed-point products and all pole-state ranges not formally proven. |
| TRIO | Oscillator sets, BLEP/ring/sync correction arrays and loops, phase/increment setup, filters | One arithmetic-validated simplification opportunity; no target measurement. Silent oscillator phases remain necessary for ring/sync contracts. |
| DRAWBAR | Arena state, block coefficients, percussion/held-note detection, partial phases/folding, gain ramps and rotary/click state | Silent partial phase advancement serves the free-running wheel design; not dead work merely because current gain is zero. |
| PHYS | Poly cap/layout selection, arena slot bounds, reset/seed behavior, model transitions, sympathetic pitch setup and render dispatch | Small/big layout guards and CPU1 reset-RNG exclusion inspected. Deep `phys_dsp.c` algorithms outside this pass. |
| FM6 | Patch sanitization/packing, per-part arena lookup, effective macro state, note/block/done/render hooks, own-envelope integration | FM6 owns its operator envelopes and has dedicated behavioral tests; no new confirmed wrapper defect. `fm6_core.c` is not comprehensively audited here. |
| Optional SLICE | Decode/window bounds helpers and note/render/amp transitions inspected selectively | `core.h:23` disables it by default; no default-image saving from deleting its source. No optional-feature build/run performed. |
| Tables / tests | Pitch generator/top-octave indexing, envelope table construction, engine descriptors and preset patterns; regress/fm6/arena coverage and sanitizer flags | Existing sparse presets/table work treated as integrated. This was not an independent audit of every generated table value or every test. |

CPU1 contracts: `fx.c:990` counts all FORMANT tracks and line 1002 keeps them on core 0 when multiple parts share breath noise. Lines 1001–1002 exclude wrapping LFO RNG draws and PHYS resets predicted by `phys_render_draws`. FM6 buses and envelope-sync coefficient arrays are per-part/per-track. These are purposeful ownership rules, not generic candidates for parallelization. No multicore hardware race proof is claimed.

## Validation evidence and limits

Private files: `build/code-review-engines-51/repro.c`, compiled `repro`, and `repro-output.txt`. Built and ran against current production includes with WSL Ubuntu and `build/host/optimization-cc` (Zig .13), `-O1 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -lm -pthread`. The reproduction completed successfully; it prints observed failures rather than asserting expected fixed behavior. The TRIO arithmetic sweep passed.

An attempted standalone signed-overflow NOISE stress fixture is retained as `noise-stress.c`, **not a successful sanitizer result**. Zig's reporting build failed to link its UBSan runtime (`__ubsan_handle_*_overflow_abort`); WSL `cc` was unavailable. No arithmetic safety conclusion follows from that failed attempt. The existing runner at `tests/run_tests.sh:207` explicitly excludes signed-integer-overflow and shift-base, so its ASan+UBSan coverage cannot establish their absence. This is a coverage limitation, not evidence that every two's-complement idiom miscompiles. A useful follow-up is narrow checked-arithmetic instrumentation for accumulators/filter products, preserving intentional unsigned phase wrapping, rather than globally rewriting DSP or enabling a noisy blanket check without triage.

Full regression, fresh target image/disassembly, deadline measurements, optional SLICE builds, hardware/audio listening, exhaustive user-sample upload concurrency, and deep FM6/physical-model cores were not run/audited. No claim of proof of no other bugs, no claimed target byte reduction, and no partition/sample-format changes are made.
