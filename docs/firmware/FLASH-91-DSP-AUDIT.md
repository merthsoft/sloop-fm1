# Flash .91 DSP audit

2026-10-28. Frozen **2.8.0.91 working-source** audit, including the uncommitted hold-delay/worker changes. Production firmware, assets, installer, version, existing reports and Git state were not changed. Only this report and private `build/flash-audit-91/dsp/` evidence were written. No flashing, publication, commits or delegation.

## Result

The preferred first review batch saves **208 linked bytes**, reaching **584,088 / 584,336 B (248 free)** without changing FORMANT. An optional exact FORMANT setup helper brings the measured combination to **584,056 B (280 free), 240 B saved**. Both keep main/low/pool usage at **97,500 / 28,928 / 335,424 B**. No resident cache or playback decoder is added. This is useful small headroom, not a multi-KiB solution.

A larger **288 B** saving was actually linked, but its FORMANT sample-loop span grows from 241 to 247 static instructions. Retain that alternative as a timing tradeoff, not the preferred throughput-preserving batch. No physical timing or stack high-water result is claimed for any candidate.

## Baseline and reproducibility

Read ARCHITECTURAL-HEADROOM, ENGINE-PERFORMANCE-INTEGRATION, HEADROOM-COMPARISON, LITERAL-STORAGE, OPTIMIZATION, the FM and VOICE/PHYS engine audits, and AMBIENT-HEADROOM-DSP. The ten .90 changes, shared ANALOG body, shared dynamic SVF setup, startup metadata/literal packing and existing master LOFI stereo control optimization are already baseline behavior; none is counted as a new gain.

The immutable input is `build/flash-audit-91/baseline/source`, not Git HEAD. The supplied manifest identifies HEAD `e3b6e4b5afcdeb641d40cf5588fc7f5d0604f781`, but the copied files include the later working changes. Private `work/` was copied from that input before experiments. The frozen `build/build-target.py` first reproduced the complete normal image; the private audit driver subsequently calls the same `build_app()`/`check()` with pinned SDK hashes, source version and private generated outputs. It does not regenerate upstream assets. Only the initial baseline ran the complete private packager; candidate results are application links. The package still in `work/build/felucca.fwsc` is the baseline package, not a candidate package. Initial and audit baseline binaries are identical to the supplied binary:

- Image: **584,296 B**; SHA-256 `c98a63741a9f16f2b9f7faa753dd31c980241a7013e56121339fbb03274552ad`.
- Main: **97,500 / 98,304 B**, 804 free.
- Low data: **28,928 / 29,392 B**, 464 free after worker-code placement/alignment. The historical 29,496-byte figure does not apply to .91.
- Pool: **335,424 / 344,064 B**, gap 8,640 B, of which **8,192 B remains reserved**; only 448 B is beyond reserve.
- `.ram_text`: 925 instructions, no calls; `.c1_text`: 160 B.

WSL Ubuntu uses `JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, pinned pi32v2 Clang 4.0.1, normal `-Oz`, and explicit **`FELUCCA_AUDIO_DIAG=0`**. Target builds were sequential. No linker region, ROM-reference rule, RAM reserve, safety check or target budget was relaxed. Host compilation uses the existing `build/host/optimization-cc`, `-UNDEBUG -O2 -fwrapv -fno-strict-aliasing`, with private Zig caches. Host execution times are not device timing evidence.

`verification.json` verifies **all 346 frozen manifest inputs unchanged**, baseline binary identity, and final linked byte identity for **303,054 B SMP_DATA**, VOWEL_F/B, SEMI_Q16, FM6_ALG and all three membrane tuning arrays.

## Ranked measured candidates

Every successful row below is a complete same-version normal link. RAM deltas are **0 main / 0 low / 0 pool** for every successful row. Combined figures come from real links, not summed independent savings.

| Rank / candidate | Image B | Saved B | Flash free B | Disposition |
| --- | ---: | ---: | ---: | --- |
| Baseline | 584,296 | 0 | 40 | Reproduced byte-for-byte |
| 1. Shared PHYS state finalization (`finish`) | 584,232 | 64 | 104 | Small review candidate |
| 2. Shared mixer accumulator clear (`accclear`) | 584,232 | 64 | 104 | Small review candidate |
| 3. Shared limiter/output conversion (`limitout`) | 584,248 | 48 | 88 | Small review candidate |
| 4. Shared PHYS dispatch, retain modal loop boundary (`dispatchguard`) | 584,264 | 32 | 72 | Small review candidate |
| **Preferred four-change batch** | **584,088** | **208** | **248** | `finish-dispatchguard-limitout-accclear` |
| 5. Separate FORMANT resonator setup (`coefhelper`) | 584,264 | 32 | 72 | Optional; extra setup call/stack |
| **Four changes + optional FORMANT helper** | **584,056** | **240** | **280** | Fully validated measured combination |
| FORMANT coefficient array loop (`coefloop`) | 584,216 | 80 | 120 | Exact output; sample span +6 instructions |
| Four changes + coefficient loop | 584,008 | 288 | 328 | Larger alternative; timing gate |
| FORMANT coefficients fused into interpolation loop (`coefused`) | 584,280 | 16 | 56 | Inferior: sample span +5; frame +24 B |
| PHYS dispatch without explicit loop boundary (`dispatch`) | 584,248 | 48 | 88 | Reject this form: hides/inlines monitored modal loop |
| TRIO conditional sentinel stores (`sentinel`) | 584,296 | 0 | 40 | No flash gain |
| Outline master dust block (`outdust`) | 584,312 | -16 | 24 | Reject as headroom work |
| Outline master DJ filter (`outdjf`) | 584,312 | -16 | 24 | Reject as headroom work |

Other linked combinations retained for reconciliation: finish+coefloop **584,152 B**; finish+coefloop+unguarded dispatch **584,120 B**; its guarded form also **584,120 B**; finish+coefused+unguarded dispatch **584,184 B**. These are alternatives, not additional savings.

Broader outlining failed the original slot check: `outlinefx` (fx_buses alone) overflows XIP by **244 B**, `outlineall` (buses/dust/DJ filter/texture) by **84 B**, and `outtexture` alone by **104 B**. These are failed-link diagnostics, not successfully extracted/installable image measurements; no larger linker was used. Patches/failure files and later batch logs remain private. Single-use inlining is not necessarily duplication: outlining these phases adds call/register traffic and often grows the image.

## Exact changes and costs

**PHYS finalization.** MODAL and MEMB end with the same operations: store active mode count; zero every out-of-band integrator; clamp each active integrator to the original bounds. `px_modal_finish` shares these operations without changing their order or bounds. The actual exciter, trigger consumption, RNG, membrane bend envelope and coefficient calculations stay live. There is one extra helper call per rendered MODAL/MEMB block, never per sample. Its prologue saves rets/r5/r4 (12 B), with no local frame. The parent coefficient functions retain their 100/116-byte local allocations. This helper's added depth is not a measured device stack watermark.

**PHYS dispatch.** MODAL/MEMB use the same block object and one common `px_modal_run` call after their distinct setup calls. All setup arguments, including the different position scales and BEND/BOW meanings, remain exact. The first experiment let Clang inline `px_modal_run` after its two uses became one: unchanged maintained checks then failed for the missing symbol and inflated `phys_render` loop estimate. The retained patch explicitly marks `px_modal_run` noinline; no budget file changes are included. Its **538 target bytes are byte-identical to baseline**. In the guarded final candidate `phys_render` local allocation is **700 B versus 924 B** originally, a 224-byte reduction; register-save counts remain the same. Model/layout resets, slot bounds and the PHYS-sized engine arena are unchanged.

**Accumulator clear.** One bounded helper clears all five accumulator arrays. Calls replace the initial core-0 clear, core-1 job clear and late-worker fallback clear at their original positions. Nothing crosses the worker join, and fallback still clears before rerendering the lost job. The target helper has no register-save/local frame and no calls inside its loop. It adds block-boundary calls, not a call per cleared sample.

**Limiter/output conversion.** The two identical DAC/USB sequences now call `master_limit_output(state,l,r,out,n)`: the original limiter, followed by the same two interleaved knee writes per sample. Their limiter states, input arrays, output destinations, USB-start reset, master scaling, FULL-mode prefilter and visualizer remain separate and in the original order. Clang folds the formerly shared `lim_block` into this helper; there is no added nested limiter call. Target register-save storage is 52 B with no local allocation, versus the old limiter's 52 B plus 4 local bytes. No per-sample helper call is introduced. The original wide products and limiter protection remain intact.

**Optional FORMANT setup helper.** `fmt_resonators` isolates vowel interpolation and all four resonator coefficient preparations, with exact original F1/F0 following, bandwidth scaling, clamps and Q30 rounding. TALK progress and RAND selection still happen in the caller, once in their original voice order. The four-stage sample recurrence, noise draws and state writeback are unchanged. Raw derived inputs, not stale parameter guesses, reach the helper each block. No persistent cache or new arena owner is introduced.

Its target FORMANT sample-loop span remains **241 instructions**, matching baseline; the alternative inline array loop has **247**. Both retain zero sample-loop helper calls. Equal static span length is evidence of bounded code structure, not equal hardware cycles. The optional helper shrinks the caller's local frame **372 -> 292 B**, but adds its own 52-byte local frame plus 52-byte register save. With the retained 44-byte `fres_coef` save, the inspected setup call-chain estimate grows **468 -> 492 B (+24 B)**, excluding caller/IRQ context. The simpler four-change batch avoids this FORMANT stack trade entirely. Hardware peak deadlines and stack high-water remain gates.

The estimator reports FORMANT cost **849 -> 785**, PHYS renderer **59 -> 59**, and modal kernel **186 -> 186**. These are static estimates; old budgets such as modal 1937 overcount branch nesting. No percentage speed claim or budget reset follows. Final PHYS modal/string/exciter/sympathetic kernels and both FM6 operator kernels are byte-identical to baseline. Master LOFI retains zero loop divisions/calls and unchanged source. Relocated call operands can change bytes in otherwise unchanged functions.

## Validation

Final optional-helper combination, with assertions enabled:

- **200 unchanged golden renders**, zero health, voice/routing or crash failures; no golden file updated. Timed CPU notes are not hardware performance qualification.
- **12 scenes x 13 split masks**, each in CORES=1, CORES=2, CORES=2/CHAOS=7 and CORES=2/CPU1_TIMEOUT_AT=700. All hashes match within each scene; flute_source retains `f6e48f0462161b93`.
- Arena/source ownership and model transitions, FX251, LOFI sampler, LOFI character and FM6 maintained suites pass. Arena compilation requires `ARENA_STATS=1`; the LOFI character test requires its private `work/build/lofi-master/` directory. Initial missing-define/output-directory failures were corrected and rerun; they were harness setup failures, not accepted DSP differences.
- Private frozen-function oracle: **80,000 PHYS block/full-state** comparisons; **60,000 FORMANT complete output/voice/shared-RNG** comparisons; **10,000 PHYS renderer/output/arena/RNG** comparisons; **100,000 accumulator-clear and limiter-output/full-state** cases. Includes zero/short/full block sizes, existing accumulator contents, large internal buses, randomized controls, model transitions, state bounds, output guards and release/trigger state. The PHYS renderer oracle retains the original dispatch and calls separately proven setup kernels.
- The full final oracle passes UBSan with `-fwrapv -fno-strict-aliasing -fsanitize=undefined -fno-sanitize=shift`. Shift checks are explicitly excluded because the original fixed-point code uses signed shifts; this is not a claim of all-UB absence. No sanitizer tolerance or output hash was changed to accept a result.
- Original target placement/ROM/RAM guards, target budgets, **25 hot-function call caps**, and master LOFI zero-division/zero-call loop checks pass unchanged. Reserve remains 8,192 B.
- Linked factory samples/numeric objects and frozen inputs verified as described above.

Evidence: `validation/`, `host-final.log`, `checks-final.log`, `stack-prologues.json`, `verification.json`; strongest final full-state result is `validation/contract-ubsan.log`. The older `contract-final` executable retained by the general check script covers the earlier PHYS/FORMANT candidate; it is not the final mixer oracle. No WASM build, physical device timing, physical stack watermark or new recovery/package fault matrix is claimed. Formats/recovery code and payloads are not changed by these DSP patches.

## Deferred larger investigations: explicitly unmeasured target ideas

**PHYS coefficient cache:** still a CPU candidate, not an established flash saving. A concrete MODAL design can retain 192 B mode coefficients plus 4 B count, six raw 32-bit mode-input keys and a validity word: **224 logical bytes appended to the original 116-byte modal state**, about 340 B total. That fits within the existing 2,656-byte string member/small slot without increasing PHYS_ARENA. This is existing engine-exclusive capacity, not spare pool to allocate again. Always run exciter/trigger/RNG, inactive-state clearing and active bounds; do not cache-return from the whole setup. MEMB must first advance bend and key its resulting effective frequency. Preserve reset/model/layout/arena invalidation. On a miss, original coefficient code remains; key tests and copies add flash, so no size saving is assumed. No target cache implementation/link is delivered here.

**FORMANT cache:** four coefficient triplets use 48 B per voice; a simple final-frequency/bandwidth key adds 32 B and validity 4 B. At four voices x three parts that straightforward design uses **1,008 logical bytes**, exceeding the 804-byte main margin if separately allocated. This is an example design cost, not a proof that all smaller keys are impossible. Existing arena adoption could avoid static growth, but FORMANT currently does not claim it: adding ownership, direct-host-call fallback, voice indexing and cache invalidation is substantive work. TALK, RAND, key tracking, F0-following and cutoff modulation make part-only keys insufficient. The delivered setup helper gains bytes without this resident state or lifecycle expansion. No flash saving is claimed for a cache.

**FM6 AMS sharing:** the nonlinear factor depends on LFO value, LFO delay, AMD and one of three nonzero sensitivities, not the note envelope. A per-part cache of three factors plus those three raw keys and validity would use **28 logical bytes** inside the existing 2,624-byte FM6 part, still below PHYS_ARENA=8,592. Preserve per-operator level multiplication/rounding and per-voice pitch-envelope work; AMD=0/amod=0 does **not** bypass the nonzero C0 curve. Raw-value keys can correctly survive patch edits with unchanged modulation values; arena adoption still requires validity handling, and direct `fm6_note_compute` tests need an explicit uncached API/fallback. This removes repeated runtime calculations, not duplicated emitted operator-loop source; extra lookup/setup code could grow flash. No cache target link or performance claim is made.

**TRIO exact reciprocal:** a bounded arithmetic design is now checked in `trio-reciprocal-proof.py`: d=inc>>15; retain d=0 -> 0; saturate when past >= d*32767; otherwise use ceil(2^32/d), multiply-high, and decrement if q*d > past. The reciprocal must represent 2^32 when d=1; blindly storing it in uint32 is wrong. The saturation guard bounds correction products below 2^32. **2,310,710 arithmetic comparisons** pass, including every nonzero denominator 1..131071 at quotient/saturation boundaries and one million arbitrary uint32 pairs. This is a mathematical prototype only: no target implementation, whole-render equivalence, linked saving or cycle result. Three setup divisions per block can lose against rare low-note crossings; a 64-bit reciprocal and correction operations have real register/stack costs. The measured conditional-sentinel experiment saves zero flash. Retain current division kernels until actual low/high pitch, sync/ring/pulse and zero-denominator target tests justify replacement.

**Reverb/delay/master/modulation:** FDN characters already share tuning rows and a common kernel; SPRING uses a genuinely different dispersive network. Merging algorithms, narrower wide-bus arithmetic or reduced updates would change sound/range and is excluded. Current dynamic SVF setup and LOFI stereo controls are already shared. Live modulation advances a common state model; its distinct pitch/amplitude/filter/pan applications remain necessary. The 13,148-byte baseline `mix_block` is largely inlined single-use work, not 13 KiB of removable duplication. Failed buses/texture outlining is concrete counterevidence to blanket outlining. The two retained mixer helpers share truly repeated setup/output operations; no larger exact DSP win was established.

## Patches, application order and reproduction

The reviewable series is private `build/flash-audit-91/dsp/patches/`:

1. `01-finish.patch` — PHYS common bounds/zeroing.
2. `02-dispatchguard.patch` — common MODAL/MEMB run site and explicit modal noinline boundary.
3. `03-limitout.patch` — DAC/USB limiter/output sharing.
4. `04-accclear.patch` — common accumulator setup. **Stop here for the measured 208 B batch.**
5. `05-formant-setup.patch` — optional additional 32 B, with the +24 B inspected FORMANT setup-stack trade.

Each patch is incremental on the preceding one. No version/build/linker/test-budget change is included. `applycheck.py` copies just the four affected files from the frozen snapshot, normalizes that private copy to LF, applies all five in order with GNU patch, and checks exact resulting candidate source. LF normalization is necessary because the supplied snapshot uses CRLF; raw GNU patch on the untouched CRLF files rejected its LF contexts. Production was not normalized or patched. Retained independent/combined directories also contain whole candidate source files and complete patches relative to the original snapshot.

Preferred four-change image SHA-256: `379235deab5357d785be9cad395c0248c08907ef4227a4867f1727d806b6421b`. Optional five-change SHA-256 is `d2d04c1a94c0fc7918348f79fa66f819877379348150f0c3c3e091aba80f51c6`. Each successful result directory has ELF, disassembly, binary, symbols, checker notes, complete source candidates and result hashes. Failed experiments retain failure records/patches; broad outlining has no valid final image.

From repository PowerShell, run one target build at a time:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec env FELUCCA_AUDIO_DIAG=0 JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 python3 audit.py baseline
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec env FELUCCA_AUDIO_DIAG=0 JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 python3 audit.py finish-dispatchguard-limitout-accclear
# Optional final candidate; this is the retained work/ source after the audit:
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec env FELUCCA_AUDIO_DIAG=0 JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 python3 audit.py finish-coefhelper-dispatchguard-limitout-accclear
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec python3 host.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec python3 oracle.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec python3 final-checks.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec python3 deliver.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/dsp --exec python3 applycheck.py
```

`contract.c` and the frozen original-function headers are private reproducible oracle sources; `contract-ubsan` is the final sanitizer executable. Recompile that oracle after changing the selected source rather than treating an old executable as new evidence. Outputs are ignored build artifacts and should be retained before cleaning build/.

Review/integration gates: rebase this exact series onto the eventual production working source; rerun the normal combined link and unchanged target checks rather than adding independent deltas; retain full-state and mixer parity checks; measure both cores' peak deadlines and stack high-water with maximal PHYS/FORMANT polyphony, heavy reverb/delay/LOFI, USB FULL recording and worker fallback. In particular qualify the optional FORMANT setup stack and the new block-boundary calls. No hardware deadline guarantee accompanies these byte savings.
