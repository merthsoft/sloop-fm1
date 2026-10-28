# Engine oscillator performance audit

2026-10-27. Read-only audit of the current dirty `merthsloop` tree, frozen as
2.8.0.89 under `build/engine-opt-oscillators/source/`. Scope: ANALOG, DIGITAL,
PHASE, LOFI instrument and NOISE. Master `vinyl.c` belongs to the parent and was
not modified or proposed for integration here. No production firmware/tests,
shared generated files/output, toolchain, Git refs or other reports were changed.
Only this report and the assigned private directory were written. No package,
flash, server, commit, push, PR or cross-chat message was produced.

**Recommended private combination saves 64 B:** 584,292 -> 584,228 B, leaving
108 B flash, with no linked RAM growth. Patch: `build/engine-opt-oscillators/combined.patch`.
The useful new changes are bounded engine-control shortcuts. They do **not**
address the reported drums + GRAIN Flute Dust + master LOFI workload directly:
none of these five engines runs for that GRAIN instrument. Preserve the parent's
master LOFI work as the immediate deadline investigation. The .87 6,333-us peak
exceeded the approximately 5,805-us half deadline; these host results cannot
establish how much device headroom has been recovered.

## Frozen baseline and evidence

Read OPTIMIZATION, DUPLICATE-CODE-AUDIT, HEADROOM-COMPARISON,
VOICE-BUDGET-EXPERIMENT and relevant ambient DSP/synthesis audits. Already
integrated shared ANALOG body, dynamic SVF setup, native clamps, cold drum
priming, GRAIN exact IMA correction and 448 half-segment checkpoints are excluded
from new savings. No applicable AGENTS.md was found in the repository or checked
ancestor/scope directories.

`manifest.json` freezes 407 firmware, test, tool and generated inputs copied from
the dirty disk tree, rather than HEAD. `status.txt` records contemporaneous dirty
paths; `snapshot-verification.json` confirms the frozen inputs remained identical.
The parent can continue editing independently. Normal .89 baseline links twice
to **584,292 / 584,336 B**, 44 B free; SHA-256
`0e80c361f982186a11b3351d321277c01b71a130334991a2703122a212c5937e`.
This is the frozen normal configuration, not a smaller diagnostic build.

All measured rows use Clang 4.0.1 from pinned WSL Ubuntu JieLi
20250324.1, `-Oz`, one firmware compilation unit, SLICE=0 and unchanged version,
linker script, generated assets and normal feature flags. `run.py` calls only
private `build_app()`/`check()`, with per-tree output paths, plus the target budget
checker. It never invokes shared wrappers, loader build or packaging. SDK input
is not needed by this app-only link. An attempted private SDK junction and the
first sandboxed WSL invocation were denied; no junction was created. The later
approved WSL execution completed entirely in the private trees.

Every target result retains `commands.json`, `size.json`, `checks.json`, symbols,
section listings, full disassembly and `budgets.txt`. All measured variants pass
image/RAM/placement and target DSP budget checks. All have the same linked RAM:
main **97,324 / 98,304 B**, low **28,928 / 29,496 B**, pool
**335,424 / 344,064 B**. Pool headroom beyond the retained 8,192-B reserve is
only 448 B. These are region occupancies, not stack/heap availability.

## Ranked recommendations

Each independent measurement below is a whole same-version target link against
the frozen baseline. Do not add savings; the two-candidate result demonstrates
layout interaction explicitly.

| Candidate | Application B | Delta from baseline | Flash free | Status |
| --- | ---: | ---: | ---: | --- |
| Frozen baseline, repeated | 584,292 | 0 | 44 | Reproduced identical hash |
| DIGITAL ratio limits | 584,260 | -32 | 76 | Exact control-rate candidate |
| ANALOG zero-detune setup | 584,276 | -16 | 60 | Exact control-rate candidate |
| LOFI WRAM means | 584,276 | -16 | 60 | Exact control-rate candidate |
| DIGITAL + LOFI | 584,260 | -32 | 76 | Measured combination, not -48 |
| ANALOG + DIGITAL + LOFI | **584,228** | **-64** | **108** | Final measured and validated combination |
| PHASE duplicate setup avoidance | 584,324 | +32 | 12 | Defer; size/stack regression |

### 1. DIGITAL: remove three invariant ratio-limit divisions per voice block

References: `firmware/src/eng_digital.c:22`, `:35`, `:36`.
`fm_ratio_inc` currently loads one of fifteen fixed ratios and divides
0x73000000 by that ratio on each of three calls per voice block. Store the
fifteen exact integer limits beside the existing ratio table; pass the ratio
index into the helper. Retain the original `% 15`, right shift, comparison and
product so parameter/source handling and Nyquist saturation are unchanged.
There is no per-note cache or invalidation protocol.

Patch: `digital-only.patch`. Actual renderer size falls **1,340 -> 1,262 B**;
its total static division sites fall **7 -> 4**. The three remaining modulo
reductions and signed velocity division still exist. No sample-loop division
was present in this engine. Local frame reservation stays **84 B** and the
renderer remains call-free. The 60-B constant table plus code/layout changes
produce the whole-image **32-B** saving, not a 78-B image saving.

Exhaustive verification compares all **251,658,240** ratio/high-increment
combinations: fifteen ratios times every 24-bit value of `inc >> 8`. Low eight
increment bits are unused by both formulas, so this covers all uint32 increment
inputs for supported ratios. Preserve generation/contract checks if the ratio
list changes; a stale limit table is the principal maintenance risk. Prefer a
small compile-time expression or generator tied to RATIO_Q8 over unexplained
hand-maintained numbers when integrating, then remeasure the resulting source.

### 2. ANALOG: skip a computed increment that zero detune overwrites

References: `firmware/src/eng_analog.c:47` through `:58`.
At DTN=0, the current setup computes d16, remainder, a pitch-table increment and
fine adjustment, then unconditionally replaces inc2 with inc1. Initialize inc2
from inc1 and perform the existing calculation only when det is nonzero. This
preserves the second oscillator phase update even with MIX=0, preserving later
live MIX/DTN changes. Do not freeze ph1 or skip its advancement.

Patch: `analog-only.patch`. The independent target saves **16 B**. The private
`analog-only/tests/engine_contract.c` compares **20,000 changing blocks**, complete
voice state and every output sample against the frozen original body; it varies
all waveforms, zero/nonzero detune, mix, LP/BP/HP taps, modulation and block sizes.
The shortcut removes only overwritten pure calculation. It consumes no RNG and
changes neither filter coefficients nor output arithmetic.

### 3. LOFI instrument: immutable WRAM means instead of a per-voice scan

References: `firmware/src/eng_lofi.c:147` through `:154`, `:193` through `:198`.
WRAM's sixteen immutable packed tables are rescanned for their mean every voice
block. Two sets of sixteen signed 16-bit means cover 1BIT and all other chips.
Use the existing DUTY-derived table index and chip selection. Preserve C's signed
division truncation: for example the 1BIT SINE mean is +2047, not +2048; the
negative VOXE mean is -2047. This is why replacing division with signed shifts
would be wrong for these means.

Patch: `lofi-only.patch`. Removes the sixteen-byte scan and final mean division
on WRAM blocks. `lofi_render` falls **1,042 -> 966 B**, adds a **64-B** read-only
table and keeps its **76-B** local frame. Whole-image saving is **16 B** alone;
combined with DIGITAL it produces no additional image saving on this link.
The means are all within int16 range. `verify_tables.py` independently checks
all 32 means with integer truncation and all fifteen ratio limits against the
frozen original tables; all comparisons pass. Tie their verification to WRAM edits; do
not introduce a runtime mutable cache or bootstrap requirement for sixty-four
bytes of constants.

The combined DIGITAL/LOFI private oracle compares **40,000 changing blocks**
(20,000 per engine), all output samples and complete voice states. Cases cycle
all chips/waves/algorithms, vary WRAM table selection and controls mid-note,
modulation, amplitude ramps and render lengths 1..CTL. Baseline source functions
are embedded as renamed reference functions in `candidate/tests/audit_contract.c`.
The oracle runs actual production renderers and original formulas, not a duplicate
simplified model. Its ratio comparison is exhaustive as described above.

## PHASE and NOISE: useful deferred/rejected work

**PHASE duplicate control setup:** `eng_phase.c:142` and `:143` always build two
coefficient structs, including WAVE2=OFF or the same wave as WAVE1. A private
prototype selects the first struct for identical waveforms, keeping both cycle
toggles, both oscillator phases and sample selection semantics intact. It skips
one pd_setup call and its divisions on those blocks. However, the whole image
**grows 32 B**, phase_render **554 -> 580 B**, and its local frame reservation
**96 -> 100 B**. Two existing per-sample pd_wave call sites remain. Its private
oracle passes **20,000 changing blocks** with all waves/WAVE2 choices, ring/mix,
sub and complete output/voice comparison. Keep `phase-only.patch` as evidence,
not the recommended near-full-flash integration. Its benefit is workload-specific
and not hardware timed.

**NOISE apparent sample-loop divide is already hoisted:** `eng_noise.c:224`
spells `crsh / 11` twice inside the loop, but actual baseline disassembly executes
one division at **0x0203879a**, before the loop starts at **0x020387d4**. The
maintained loop estimator reports zero NOISE loop divisions. A textual hoist is
not a new speed finding; a private unbuilt `noise-only/` prototype is retained
only as rejected exploration. NOISE color poles intentionally stop advancing
when COLR=0; advancing/skipping poles under another condition changes later
control transitions. ANLG/DUST consumes one noise32 result per sample even when
its visible contribution is zero: retain that consumption and impulse decision.
The META DC blocker and SVF +4096 rounding distinguish this filter from ANALOG.
Do not merge them merely because their equations look similar.

**ANALOG BLEP divisions remain real hot work:** `dsp.c:38` through `:54` uses
phase/denominator division only near wrap edges; pulse evaluates two saw edges.
The target static estimator sees eighteen division sites across mutually
exclusive analog waveform paths, not eighteen executed divides every sample.
An approximate reciprocal, lower interpolation quality, removing an oscillator,
reducing filter updates or dropping antialiasing would change sound. No such
tradeoff is proposed. An exact reciprocal/correction implementation needs its own
exhaustive proof, generated-code analysis and target timing before adoption; it
is not included in the measured patches.

**Avoid broad loop cloning, helper outlining and restrict:** DIGITAL algorithm
switches and ANALOG waveform/filter tests are block-invariant in C, but cloning
entire sample loops spends scarce flash and can increase spills. The existing
ANALOG sharing and SVF-call boundaries are already measured/integrated work.
PHASE's out-of-line pd_wave is a real per-sample call, but force-inlining/cloning
without a controlled whole-image result is not a recommendation. Prior ambient
DSP audit already rejected NOISE soft-knee outlining for a tiny size saving and
new sample-loop call. Output/state aliasing is explicitly allowed by engine
comments; do not add restrict to make it disappear. Current local reservations
(ANALOG 112 B, DIGITAL 84 B, PHASE 96 B, LOFI 76 B, NOISE 128 B) exclude saved
registers, callees and interrupts and are not stack high-water measurements.

None of these five engines needs a new shared arena allocation for the proposed
changes. GRAIN/FM6/PHYS/DRAWBAR own the existing per-part union by engine identity;
borrowing that arena requires ownership/adoption work and supplies no advantage
for these tiny immutable control tables. Do not count its occupancy as newly
available general RAM.

## Validation and integration

The DIGITAL+LOFI candidate passes all **200 unchanged audio goldens**, zero
health/routing/budget/crash failures, `auto_split_test`, engine pitch/ownership
(**14,838 checks**) and `lofi_voice_isolation_test` (all master LOFI types preserve
Flute Dust chord gates with drums on host). `mix_split_test` passes **11 scenes x
13 splits**, separately under CORES=1, CORES=2, chaotic worker scheduling and the
real lost-job timeout fallback at job 700. Scenes include source/preset changes
while notes sound, shared RNG exclusions, MORPH and the reported flute/LOFI load.
Host deadlines are not simulated. The original private arithmetic test compile
used nonexistent ENGI_LOFI/ENGI_DIGITAL constants; correcting them to the actual
stable indices 3/1 makes the separate `contract-host` run pass. This was a harness
error, with no production fix. The first results.json retains the failed compile
honestly; use the corrected contract results as evidence.

Pinned Zig 0.13 host tests use -O2 with assertions. No device cycle benchmark,
stack watermark, ThreadSanitizer or new sanitizer qualification was performed.
Static loop-estimator changes (DIGITAL 340 -> 329, LOFI 128 -> 112) can include
register allocation and loop-span detection; they do not prove a percentage CPU
improvement. The controls removed here are block-rate. RNG/source bytes, upload
paths, parameters, presets, voice limits/admission, 95% two-half guard, ownership
and upstream formats remain unchanged in the private candidates.

The final three-change combination in `combined/` passes every test listed above,
including the corrected arithmetic oracle and the ANALOG differential oracle.
`combined-host/results.json` records seven successful compilations and ten successful
executions (the split test has four modes). `combined-result/` retains its full target
evidence. Its repeat has the identical size and SHA-256. Its SHA-256 is `0a8dd3c2868fd42e835f83f24f56b13d76a31041292386a3edc8cd2c732b39a0`.
ANALOG's emitted body falls 1,742 -> 1,724 B and its local frame stays 112 B;
DIGITAL/LOFI retain the frame reservations reported above. No sample-loop
helper call is added. The final combination's 64-B saving is measured, not the
sum of isolated function/table estimates.

Integrate only the selected narrow patch into the parent's final source,
repeat its own same-version normal whole-target link, and retain differential
checks as maintained tests. Table edits must carry a mean/ratio oracle. Re-run
same-boot hardware counters and listening on the motivating workload; do not
claim these engine-control changes solve a master/GRAIN deadline failure.

Reproduction (from PowerShell; all writes remain private):

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-oscillators --exec env FELUCCA_SLICE=0 python3 run.py baseline-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-oscillators --exec env FELUCCA_SLICE=0 AUDIT_SOURCE=combined python3 run.py combined-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/engine-opt-oscillators --exec env AUDIT_SOURCE=combined AUDIT_NAME=combined-repeat AUDIT_TESTS=regress,mix_split_test,auto_split_test,engine_pitch_ownership_test,lofi_voice_isolation_test,audit_contract,engine_contract python3 host.py
```

Preserve the ignored private patches and evidence before cleaning build storage.


## Parent integration follow-up

The recommended ANALOG/DIGITAL/LOFI combination are integrated in .90. The actual combined normal link is
584,228 B (108 B free), with unchanged static RAM. DRAWBAR caching fits when
combined with the other selected changes. Full combined validation and the
deferred findings are recorded in [the integration report](ENGINE-PERFORMANCE-INTEGRATION.md).
The original audit measurements above remain independent baseline evidence.
