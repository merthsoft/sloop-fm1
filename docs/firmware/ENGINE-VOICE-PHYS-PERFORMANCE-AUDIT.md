# VOICE / PHYS performance audit — frozen dirty .89

Read-only production audit, 2026-10-27. Production firmware, tests, generated
headers, toolchain and Git refs were not edited. Experiments and patches are in
`build/engine-opt-voice-phys/`; this report is the only change outside it.
Master `vinyl.c` and scheduling were excluded from candidate ownership. Existing
.88 GRAIN IMA correction and half-segment checkpoints are not new findings.

## Result and scope

One small measured integration candidate: use the existing ordered native clamp
at six MODAL/MEMB state-bound sites, saving **32 linked image bytes**, with no
static RAM change. This does not establish a meaningful improvement in the
reported drums + GRAIN Flute Dust workload: VOICE/PHYS are not that workload's
active engine. Do not describe it as a fix for the measured 6,333-us / 62-shed run.
The 16/24 admission setting and 95% two-half guard remain untouched.

The larger plausible speed opportunity is caching stable PHYS modal-bank
coefficients within unused capacity of each existing small PHYS slot. This is
an investigation recommendation, not an implemented or measured saving.
FORMANT setup caching is less attractive under the frozen build's tight main
RAM margin and its many independent per-voice modulation inputs.

## Frozen baseline and measurement method

The dirty `firmware/`, `tests/` and `tools/` were copied before experiments;
`phys_dsp.original.c` preserves the original candidate file. The remaining
firmware snapshot stays unchanged. `baseline-source-hashes.json` records the
baseline firmware hashes (substituting that preserved original for the modified
experimental file). `dirty.patch` is contextual tracked-diff evidence; copied
untracked firmware files are present in the snapshot too.

Reviewed OPTIMIZATION, DUPLICATE-CODE-AUDIT, HEADROOM-COMPARISON,
VOICE-BUDGET-EXPERIMENT and the earlier AMBIENT DSP/synthesis audits. No applicable
AGENTS.md was found at the repository or inspected ancestors. Prior coefficient
sharing, native clamp work elsewhere and engine-arena ownership are integrated
baseline work; only the six missed PHYS sites are new here.

`run.py` calls private `build_app()` and `check()` only; no shared wrapper,
generation, loader, SDK mutation or packaging. Generated headers were copied
into the private tree. Each result retains complete ELF, image, disassembly,
symbols, sections, checks and command arrays. Target: pinned JieLi Clang 4.0.1,
pi32v2, normal production -Oz flags, FM-1_900, source version 2.8.0.89,
boot tables/presets/literals enabled, **FELUCCA_AUDIO_DIAG=0**. WSL Ubuntu and the
specified installed toolchain were used. Build checks and target budgets pass.
This is a frozen dirty-source measurement, not the parent's later finalized .89.

| Independent variant | Whole image B | Delta B | Free B |
|---|---:|---:|---:|
| Frozen normal baseline | 584,292 | 0 | 44 |
| All PHYS clamps use common helper | 584,292 | 0 | 44 |
| String base-delay product hoisted | 584,324 | +32 | 12 |
| **Six modal-only clamp sites** | **584,260** | **-32** | **76** |

All four links: main `.data + .bss` **97,324 / 98,304 B**, low RAM
**28,928 / 29,496 B**, pool **335,424 / 344,064 B**. No static region changes.
These margins belong to this snapshot, not older .88/diagnostic reports.
Image size, not a sum of function estimates, is the savings claim. Variants
interact and must not be summed.

## Ranked recommendations

### 1. Optional small integration: six modal state clamps

Patch: `build/engine-opt-voice-phys/modal-clamp.patch`.

Source sites: `firmware/src/phys_dsp.c:334`, `:395`, `:435`. Replace only the four
MODAL/MEMB integrator clamps and two modal exciter-state clamps with the existing
`clamp` macro from `firmware/src/dsp.c:28`. Bounds are constant, signed, ordered
and within int32. The old PHYS helper has the same lower-first fallback contract.
The shared macro already selects native min/max for these ordered bounds and
portable C on host/WASM. Input expressions are evaluated once; no new helper,
state, coefficient rounding, RNG draw, render order or format is introduced.

Whole-function bytes (baseline → candidate): px_modal_block 660→646,
px_memb_block 676→662, px_modal_run 548→538. These gross reductions are not an
alternative linked-size total. String render stays 1,004 B; FORMANT stays 1,884 B.
Local stack allocation is unchanged: px_modal_block 100 B, px_memb_block 116 B,
px_modal_run 180 B, excluding saved registers and callees. No hardware stack
high-water, execution-cycle or deadline measurement was made.

Evidence:

- Existing 200 goldens: zero changed, zero health/voice failures, zero crashes.
  `tests/regress_audit.c` privately retains the existing golden, health and voice
  checks and omits only CPU timing jobs/reporting. No golden was updated.
- Real split suite: 11 scenes × 3 splits, CORES=2/CHAOS=7 and worker timeout at job
  8, all match unsplit. Includes FORMANT shared noise, PHYS model/layout switches,
  pitch/LFO state, morph/source transitions and flute/LOFI. Logs:
  `modal-split.log`, `modal-timeout.log`.
- Existing target clamp test: **20,726 signed boundary/random cases** interpreting
  actual emitted target registers pass. Host goldens alone cannot exercise the
  target native asm. This is still not device execution.
- All target budgets and linker placement checks pass. The estimator's modal
  cost changes **1937→186**, despite only four fewer total instructions and no
  changed modal recurrence. Conditional branch-layout spans change its nesting
  heuristic. **Do not claim a 90% speedup or blindly reset that budget.**

Integration: apply only this patch if 32 B is useful, retain the existing target
clamp regression, rebuild the parent's actual final normal image and recheck
budgets. A device timing comparison is necessary for any speed claim.

### 2. Higher-value follow-up: stable PHYS modal-bank coefficient cache

Sources: `phys_dsp.c:293` and `:352`; twelve-mode coefficient loop `:309`,
state bounding `:331`; `eng_phys.c:51`, `:98`, `:133`, `:207`.

Every render recomputes modal frequencies, gains and two reciprocal divisions
per active mode through `px_svf_coef` (`phys_dsp.c:162`), even for settled notes.
At CTL=32 / 44.1 kHz that is about 1,378 control blocks per second per voice,
up to 24 mode-coefficient divisions per block, in addition to other setup work.
MEMB has up to ten modes and a changing bend envelope. This is control-rate
work, not a per-sample division bug.

Host ABI sizing (all relevant fields are scalar fixed-width arrays): modal state
116 B, string 2,656 B, sympathetic 4,292 B; small slot 2,664 B, big slot 4,296 B,
PHYS/engine arena 8,592 B. Modal block coefficients are 228 B. Thus there is ample
logical capacity inside each **existing modal use of a small slot**, without
borrowing memory from GRAIN or growing the largest union member. Preserve the
`sizeof(px_modal_t) <= sizeof(px_string_t)` assertion and verify target layout.
This is not new free pool memory and cannot be counted again as GRAIN slack.

Cache only stable mode coefficients/count and required stable exciter data, keyed
by every exact integer input used by that calculation. Leave the following work
live on every block: trig/strike consumption, bow/noise RNG, MEMB bend-envelope
advance, inactive-mode clearing, integrator bounds and any state reset. A simple
return from `px_modal_block` would be wrong. Include model/validity in reset
semantics and invalidate on the existing arena/model/layout reset. Morph, DRIFT,
pitch fine/glide, cutoff envelope, velocity and release changes must miss when
they change effective input. Do not freeze coefficients for a timed interval.

The existing `px_symp_block` key cache (`phys_symp.c:68`) is useful local precedent;
its scope is only the sympathetic feedback coefficients. It does not already
cache the main string or modal bank. Prototype the narrow bank cache privately,
compare full state/output with settled and per-block-changing controls, and then
measure normal whole-link cost plus device cache-hit/miss peaks. No patch or
flash/speed/RAM saving is claimed for this unimplemented follow-up.

### 3. FORMANT setup: investigate only with a measured control-key design

Sources: `eng_formant.c:101` setup; vowel interpolation `:119`; four resonator
coefficient builds `:142`; source reciprocals `:154`; sample recurrence `:178`.
All four filters, source slope reciprocals and gain mapping are rebuilt every
voice/block. Coefficients depend on separate combinations of TALK progress,
random vowel bits, cutoff, SHIFT, pitch, increment, Q, BUZZ/shape and velocity.
A cache keyed only by eight engine parameters would fail live modulation.
F4 depends only on ratio/bandwidth scale, so a narrow F4 cache is a simpler first
probe than an entire render-control cache. TALK=0 can also avoid calculating the
unused second vowel, but needs an actual target size comparison: adding branches
and copying arrays may offset six small interpolations. Neither was implemented.

FORMANT's sample loop has four rounded Q30 three-product recurrences and all
filter history is already local scalar state. Reassociation into separately
rounded products or a two-product difference form changes samples. Keep rounding
points. The target static report counts two divides inside broad loop spans;
source `rp/rn` divisions are setup, while `blep` has edge-dependent work. Do not
report that number as two dynamic divisions on every sample.

A per-voice control cache needs RAM and validity ownership. FORMANT does not use
the PHYS/GRAIN/FM6 arena, so it cannot freely borrow that arena while those other
parts play. The frozen normal main-RAM margin is only 980 B; do not casually grow
voice_t or add a global cache. No measured implementation is recommended here.

## Rejected experiments and practical constraints

- **Blanket PHYS clamp replacement:** zero linked saving; px_string_run grows
  1,004→1,032 B, 317→323 instructions, estimated loop cost 260→266. Native clamp
  operand/register constraints change allocation. The existing quick split suite
  passed CORES=1, CORES=2, CHAOS=7 and timeout for this candidate, but it is rejected.
  Patch retained as `clamp.patch`; use modal-only instead.
- **Hoist string delay × compensation from the sample loop:** exact when B and S
  are distinct, as real callers provide. Private differential test compares
  every output and complete px_string_t over **163,840 cases**: pitches across
  0..2047, curved/dispersion paths, bowed/plucked exciters, retriggers, n=0..32,
  resampling and long state evolution. All pass; full 11×13 chaotic split matrix
  passes. Yet linked image grows 32 B, string body 1,004→1,030 B and instruction
  count 317→330; static loop cost 260→261. Increased lifetime/register pressure
  defeats this obvious source hoist. No hardware speed proof; reject at 44 B free.
  Evidence: `delay-hoist.patch`, `string_diff.c`, `string-diff.log`, `delay-split.log`.
- Do not skip dispersion RNG when noise_amt=0: px_string_run still advances rng
  and disp, affecting later parameter changes. Do not skip FORMANT breath draws
  based on output amplitude; formant_nz is shared across voices/parts. Note-on
  also consumes this stream. Keep current shared-RNG and PHYS reset exclusions.
- Reordering the modal recurrence mode-major could retain two state scalars but
  needs an exact wide accumulator per output sample (32×8 B) or equivalent. The
  current 180-B local modal frame already spills scratch. No claimed gain; avoid
  a broad rewrite without stack/cycle evidence.
- Model switches can clear 2.7–4.3 KiB per fresh slot; changing to lazy clearing
  of delay lines would add hot read checks and validity state. Keep existing
  reset/RNG ownership; no safe measured transition optimization was established.
- Lower mode count, lower resonator precision, approximate reciprocals and less
  frequent coefficient updates are sound-changing alternatives, outside this
  exact-output recommendation. No feature/polyphony/sample-format cuts proposed.

## Reproduction and limits

From the private tree, `python3 run.py NAME` uses its current experimental source.
Apply the chosen retained patch to `phys_dsp.original.c` before reproducing a
specific variant. Baseline image SHA256:
`0e80c361f982186a11b3351d321277c01b71a130334991a2703122a212c5937e`.
Modal-only image SHA256:
`bdc583e94e1e1d7f6adbfa49f50ec21636ac103ad094f8a98a0c654ad1c8de86`.
Pinned Zig 0.13 host tests use private local/global caches and outputs. The first
shell test script's last redirect had CRLF and did not run target-clamp; the
LF-normalized modal script reran it successfully. No result relies on that failed
invocation. The standalone delay differential and full split results are recorded
separately; no sanitizer or hardware timing result is claimed.

Only the modal-only patch is a measured size recommendation. Preserve its modest
scope, rebuild the parent's combined source, and judge deadline improvement with
same-boot on-device SHEDS/LATE/MAX US plus listening. Independent savings here
must not be added to master-LOFI or other engine experiments without a joint link.


## Parent integration follow-up

The six modal-only clamp sites are integrated in .90. The actual combined normal link is
584,228 B (108 B free), with unchanged static RAM. DRAWBAR caching fits when
combined with the other selected changes. Full combined validation and the
deferred findings are recorded in [the integration report](ENGINE-PERFORMANCE-INTEGRATION.md).
The original audit measurements above remain independent baseline evidence.
