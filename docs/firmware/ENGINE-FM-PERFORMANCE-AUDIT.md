# FM6, TRIO and DRAWBAR performance audit

2026-10-27. Read-only audit of the dirty `merthsloop` worktree, frozen as **2.8.0.89**, HEAD `d352b6715bd95f0a972e733141a6e105f1eaa6dd`. The best ready candidate replaces FM6's repeated carrier scan with an exact table: **zero linked flash/RAM growth**, fewer target instructions. A DRAWBAR settled-control cache is behaviorally promising but adds **48 image bytes**, exceeding this snapshot's slot by **4 B**. Neither candidate addresses the reported drums + GRAIN Flute Dust + master LOFI workload when these audited engines are inactive.

Only this report and `build/engine-opt-fm/` were written. Production firmware/tests/tools, shared generated/build outputs, Git refs and other reports were untouched. No package, loader, flashing, serving, PR or messaging action was performed. Master `vinyl.c` is excluded; integrated GRAIN IMA arithmetic/checkpoints are not new findings here.

## Frozen source and measurement

`build/engine-opt-fm/snapshot/` contains dirty firmware, tests, tools and a private copy of generated headers. `snapshot-sha256.json` freezes 382 firmware/tests/tools files; `verify.py` verifies they remained unchanged. Copies happened before candidate edits and experiments. This is a captured working-source comparison, not committed HEAD or a claim about the parent's later .89 binary. Reads included OPTIMIZATION, DUPLICATE-CODE-AUDIT, HEADROOM-COMPARISON, VOICE-BUDGET-EXPERIMENT, VOICE-BUDGET-AUDIT and relevant earlier asset/space audits. Their integrated/rejected work is not counted again.

Private `run.py` imports only the copied build module, invokes app compile/link/check and target budgets, and records commands, ELF, symbols, disassembly, sections, hashes and checks in each result directory. It never invokes shared build wrappers or package generation. Target: installed WSL Ubuntu JieLi 20250324.1, Clang 4.0.1, `-target pi32v2 -Oz`, production boot-table/literal/preset settings, `FELUCCA_SLICE=0`, normal About configuration. No diagnostic-size inference. Host tests use installed Zig 0.13, `-O2 -UNDEBUG`, private caches and executables.

| Same-version isolated variant | Complete image | Delta | Real 584,336 B slot free | Main / low / pool RAM |
|---|---:|---:|---:|---|
| Frozen dirty .89 | 584,292 B | baseline | 44 B | 97,324 / 28,928 / 335,424 B |
| FM6 carrier lookup | 584,292 B | 0 B | 44 B | unchanged |
| DRAWBAR control cache, analysis link | 584,340 B | +48 B | **-4 B** | unchanged |

RAM capacities are 98,304 / 29,496 / 344,064 B respectively. Thus this frozen image has 980 B ordinary static RAM margin, 568 B low RAM margin, and 8,640 B pool gap including the required 8,192 B reserve. These totals differ from historical reports and must not be replaced with older margins. The DRAWBAR candidate initially fails the **unmodified real linker** by four bytes. Only its private analysis linker then extends XIP by 4 KiB to obtain the full linked size and RAM totals; the normal slot check still explicitly fails. This is not an installable candidate. The submitted cache patch does not contain that linker relaxation. Variants are independent; no savings are summed.

Image SHA-256:

- Baseline: `0e80c361f982186a11b3351d321277c01b71a130334991a2703122a212c5937e`.
- Carrier: `ae1bfba3c7342d2ffe525df085ceccbea9201abfa034a01d84099c5d6e4743a7`.
- DRAWBAR analysis: `735469138c8fa8c9e440671ebcecc5ecf59f7aa5c55fc3a78de85b8cf4cbdf52`.

## Ranked recommendations

### 1. FM6 carrier mask table: ready for integration review

Frozen references: `firmware/src/fm6_core.c:304`, `:513`, `:541`; `eng_fm6.c:228`, `:327`. Patch: `build/engine-opt-fm/carrier.patch`; full candidate: `carrier-source/`.

`fm6_carriers` scans six immutable algorithm bytes at patch sync, every sounding voice computation and voice completion check. The target compiler retains that scan as a 46-byte function: loop, six table loads, conditional flag handling, shifts and register save/restore. A 32-byte mask table changes the function to **16 code bytes / four target instructions** (mask index, load table address, byte load, return). Linked image size is identical after table and alignment costs; call sites remain shared. This removes repeated work at the actual voice-control boundary and reduces helper stack traffic. It does not change the six-operator sample kernels.

`verify.py` derives all 32 masks independently from the frozen `FM6_ALG` rows and compares every table value. Both implementations index with `a & 31u`, proving equivalence for all uint32 algorithm aliases. Carrier identity, modulator-level application, done decisions, algorithms, patch edits, RNG and formats are unchanged. No writable state, init dependency or cross-core ownership is added. An integration should retain the short derivation/check near maintained FM6 tests to avoid two independently edited algorithm descriptions drifting apart.

Validation on the carrier candidate:

- `fm6_test`: all checks pass, including 32 algorithms, envelopes/retriggers, macros, AMS 1..3 with AMD 0/99, factory formats, synthesized SysEx and simulated bank operations.
- `regress`: **200 unchanged golden renders**, zero health/voice/routing failures or crashes. Timed host CPU warnings are not target cycle evidence.
- `mix_split_test`: **11 scenes × 13 splits**, separately CORES=1, CORES=2, CORES=2/CHAOS=7 and CORES=2/CPU1_TIMEOUT_AT=700: all pass. Scenes include three FM6 parts and real source/engine changes.
- `arena_test` with ARENA_STATS: all checks pass, including DRAWBAR/GRAIN/FM6/PHYS reuse and PHYS model transitions.
- All target placement, budget and hot-helper checks pass. FM6 operator loop metrics are unchanged (`fm6_op_run` 72 loop instructions, feedback 26, no loop divisions/calls). Broad render-span estimators omit the benefit inside the called carrier helper; do not interpret their unchanged cost as a cycle measurement.

Hardware timing and instruction-cache effects remain unmeasured. The static helper reduction is real target code evidence, not a quantified percentage speedup or deadline qualification.

### 2. DRAWBAR settled registration/control cache: exact prototype, defer under current budget

Frozen references: `eng_drawbar.c:94`, `:133`, `:199`; `engines.c:31`. Patch: `build/engine-opt-fm/drawbar-cache.patch`; candidate `drawbar-source/`.

Each block recalculates nine registration levels, their sum/normalization division, nine normalized gains, percussion settings, click level and drive compensation division. Those values depend only on E0..E6. The patch remembers the seven **raw** parameter values and an initialization bit inside the existing part arena, recomputing these values on first use/change. Rotor speed easing, phase advance, amplitude/pitch modulation and wet easing still run every block, including while turning OFF. E7 remains live. No sample-loop test is added.

The settled path compares seven int16 values and bypasses the registration/gain/division work. Changes pay the original calculations plus cache comparison/store overhead. Cache state grows the logical DRAWBAR part **832→848 B** (track state 96→112 B), inside the existing **8,592 B** arena, so linked pool/main/low RAM deltas are zero. The existing DRAWBAR arena static assertion still passes. Invalidating by arena zeroing follows existing source-switch ownership and does not add a new lifecycle mechanism.

Whole-link result: **+48 B**; `drawbar_block` grows from 554 to 590 function bytes, with other placement/alignment costs included in the full-image delta. This exceeds the current slot. Keep as a CPU/flash tradeoff after other independent space work; do not silently consume the pool reserve or change admission. There is no hardware proof that the saved setup time matters more than its branch/fetch costs.

Private `drawbar_contract.c` embeds the original control function as an oracle. Across **200,000 blocks**, deterministic legal parameter changes, long settled spans, all rotor modes and repeated arena invalidation, it compares every original track-state byte from `g` through the final rotor modulation fields: exact match. Separate candidate runs pass all 200 audio goldens and the full arena/source-switch suite. The FM6 candidate's split results do **not** qualify this separate DRAWBAR cache; run its full split/timeout matrix before integration, then hardware peak/deadline/stack tests. No sound-changing approximation is used.

### 3. FM6 shared modulation work: useful next experiment, not measured savings

Frozen `fm6_core.c:507–534`, `eng_fm6.c:302` already share LFO value/delay once per part. However, each voice repeats PMD/sensitivity/AM depth arithmetic, and each AMS-enabled operator repeats the nonlinear amplitude factor. Within one part/block the factor depends only on LFO value/delay, patch AMD and the operator's **four-valued AMS index**; it does not depend on voice envelope level. Compute at most the three nonzero AMS factors per part/block and reuse them across operators/voices, leaving the level multiply at each voice/operator. Store cache in FM6's arena (currently 2,624 of 8,592 B), not a new global per-part allocation. Assert FM6 still fits PHYS_ARENA, as DRAWBAR/GRAIN already do.

This has greater potential when many operators/voices use AMS, but needs a separate same-version linked experiment and cache lifecycle tests. The block must compute after patch sync; a new edited patch, macro reset, note start, arena adoption and direct core-level test calls must all observe the original arithmetic/order. A lazy per-compute cache is a simpler initial experiment but saves less across voices and adds per-voice setup. Do not move mutable envelope progression or per-voice pitch envelopes to the shared cache.

**Do not skip AMS processing merely because AMD or amod is zero.** FM6_AMS_C0 is nonzero, so the original curve applies a small attenuation even at zero modulation; the maintained AMS tests explicitly document this. Removing it is a sound change. Fixed-mode operators also recompute `fm6_freq(base)` each block; precomputing at note init could reuse the existing op.freq field, but may duplicate target code, and live edited-patch/base semantics need exact tests. Neither idea has a claimed size/time result here.

### 4. TRIO: preserve correction arithmetic; prioritize bounded setup work

Frozen `eng_trio.c:76`, `:120`, `:193`, `:217`, `:267`. Target `trio_pass` retains three division sites in wave loops; ring/sync corrections also call the inline fraction calculation. `trio_frac` divides by **inc >> 15**, clamps at 32767 and explicitly returns zero when that denominator is zero. Replacing it with a naive reciprocal multiply changes correction positions at quotient boundaries; shifting past or dividing by inc first is not equivalent. Exact reciprocal-plus-correction is a possible experiment, but needs per-oscillator setup divisions and correction multiplies. At low notes crossings are sparse, so setup every 32-sample block can cost more than the existing occasional division. Measure low/high notes and pulse/sync/ring separately; do not assume an unconditional win.

The filter coefficient division is already once per voice/block, and its cutoff includes **voice-local random drift**. Sharing filter coefficients across voices would change sound/RNG behavior. Oscillator 3 can have zero audible gain yet still drives ring/noise phase behavior. Skipping its pass indiscriminately changes phase or RNG consumption. Current waveform dispatch already sits outside per-sample wave loops; avoid turning it into a generic sample dispatcher.

A smaller future experiment is initializing `wx[]` only for sync sets. Currently every block writes 32 sentinel values even when the array is never read. This is exact if all sync consumers retain full initialization, but an extra loop/branch may grow code; target-measure first. Keep `mix[]`, `cr[]`, the `cr[n]` boundary and the one-sample-delayed output semantics intact. No TRIO patch or measured saving is claimed in this audit.

## Rejected shortcuts and maintenance

- DRAWBAR already advances phases without lookup/multiply for silent partials and uses a separate constant-gain loop. Those are existing wins, not new findings. Dropping silent wheel phase advancement breaks later drawbar/rotor changes. Combining nested Q15 amplitude products changes rounding. Removing click work must preserve its local noise/high-pass/envelope progression.
- FM6 operator loops already dispatch modulation/add flags outside ordinary sample loops. Splitting feedback add/replace loops could remove one per-sample selection, but duplicates a loop and is unattractive at 44 B flash margin without a measured same-version result. No broad assembly, restrict promise or compiler flag is proposed.
- Arena slack is engine-exclusive existing capacity, not independent free allocations that can be summed between engines. Preserve arena ownership/zeroing and its reserve. Neither eight FM6 slots nor weighted admission/95% two-half protection changes.
- Comments are stale: `eng_fm6.c` describes six voices and 244-byte notes; actual FM6_POLY is eight and fm6_note_t is 296 B (part 2,624 B with buses). `engines.c` still says six notes; `eng_drawbar.c` describes side arrays/unused voice_t despite arena adoption. Correct these alongside a future engine change; comments are not reliable memory-budget evidence.

## Integration and reproduction

Recommend reviewing **carrier.patch only** as a small independent speed/clarity change. Rebase onto the parent's completed .89 source, regenerate the table check from FM6_ALG, repeat the normal whole link and maintained FM6/audio/split/arena tests, then qualify device timing. It should not be presented as the GRAIN + master LOFI dropout fix. Keep DRAWBAR caching optional until funded and hardware-profiled. Preserve existing edited FM6 payloads, sources/uploads, formats, RNG order, polyphony and ownership.

Private reproduction from repository root (WSL access may require sandbox escalation):

```powershell
python build/engine-opt-fm/verify.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-fm --exec env FELUCCA_SLICE=0 PYTHONDONTWRITEBYTECODE=1 python3 run.py baseline-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-fm --exec env FELUCCA_SLICE=0 AUDIT_SOURCE=carrier-source PYTHONDONTWRITEBYTECODE=1 python3 run.py carrier-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-fm --exec env AUDIT_SOURCE=carrier-source AUDIT_NAME=carrier AUDIT_TESTS=fm6_test,regress,mix_split_test,arena_test PYTHONDONTWRITEBYTECODE=1 python3 host.py
```

Private patches/evidence are in ignored build storage; retain them before cleaning build/. No physical target-cycle benchmark, target stack high-water, or exact arbitrary edited-payload audio differential beyond the exhaustive carrier identity proof and maintained FM6 format/macro tests was performed. No output-changing tradeoff is recommended.



## Parent integration follow-up

The carrier lookup and DRAWBAR control cache are integrated in .90. The actual combined normal link is
584,228 B (108 B free), with unchanged static RAM. DRAWBAR caching fits when
combined with the other selected changes. Full combined validation and the
deferred findings are recorded in [the integration report](ENGINE-PERFORMANCE-INTEGRATION.md).
The original audit measurements above remain independent baseline evidence.
