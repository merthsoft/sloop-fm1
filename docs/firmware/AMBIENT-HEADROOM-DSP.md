# Ambient headroom: DSP C structure audit

Audit date: 2026-10-24. Scope: engines, voice/modulation, FX buses, master LOFI coloration and reverb/delay variants. **Best measured candidate saves 1,856 application bytes**, taking the exact .58 source from **583,388 to 581,532 B**, with **2,804 B free** in the unchanged 584,336-byte slot. All static RAM regions are unchanged. This is a conditional integration candidate: host output/state equivalence is established for the tested vectors, but its shared analog loop needs hardware deadline and stack validation before adoption. It is not a demonstrated speed improvement.

No production source, shared generated asset, shared build output, installer, version or Git state was modified by this audit. The only non-scratch deliverable is this report. All experiments use private copies under `build/headroom-ambient/dsp/`; no package or flash operation was run. Other concurrent work and any parent commit do not enter the frozen comparison.

## Frozen inputs and reproduction

The current dirty `firmware/`, `tools/`, `tests/` and `build/gen/` were copied from disk, not exported from HEAD. `manifest.json` records SHA-256 for **321 inputs**; `snapshot-verification.json` verifies every original private input unchanged after restoring the private baseline. The added differential harness is separate from those original inputs. The initial and repeated private target images exactly reproduce the requested .58 baseline:

- Baseline image: 583,388 B; SHA-256 `f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`.
- Recommended prototype: 581,532 B; SHA-256 `f6a461d4f07ce1e11bb59763033a8c7bc7f1510197f0e9dfdbb1865379fe73e8`.
- Both baseline and candidate repeated with identical respective image hashes.

Target toolchain: WSL Ubuntu, `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, Clang 4.0.1, `-target pi32v2`. Production flags remain `-Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, boot tables and boot presets enabled, SLICE disabled, `FM-1_900`, source version unchanged. Full compile/link/extraction argument arrays are saved per result in `commands.json`; executable hashes and compiler output are `toolchain.json` and `compiler.txt`. Linker regions were not enlarged. The driver invokes only private `build_app()` and `check()`, never normal generation, loader building or packaging. The preset probe/packer runs inside the private tree.

Reviewed prior architectural headroom, optimization, redundancy, compiler flags and link audits, plus relevant audio/engine review findings. Existing preset/table boot restoration, limiter/texture/punch BSS initialization, sample sharing, TRIO fine-factor reuse, DSP_INLINE policy, arena sharing and widened FREEZE arithmetic are already integrated and are not counted again. The .58 above-unity sampler gain fix remains untouched.

From the repository in PowerShell, the retained reproduction entry points are:

```powershell
# Private baseline is restored; this does not run a shared build.
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/dsp --exec python3 run.py baseline-repeat
# Final candidate source is retained separately in tap-source/.
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/dsp --exec env AUDIT_SOURCE=tap-source python3 run.py analog-tap-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/dsp --exec env AUDIT_SOURCE=tap-source LOG_PREFIX=tap-host- python3 host.py
python build/headroom-ambient/dsp/costs.py
python build/headroom-ambient/dsp/verify.py
```

`experiment.py NAME` resets the private source's five experimental files before selecting an initial variant; `direct.py` refines analog-shared. `tap.py` and `tap-tests.py` are one-time preparation scripts requiring absent destination trees; the already prepared `tap-source/` should be reused. `summary.json`, section/symbol listings, disassembly, checks and patch files preserve the measured results. Do not rerun `snapshot.py` over the existing snapshot.

## Ranked target results

Every row is a complete target link against the same baseline. Alternatives are **not additive**.

| Candidate | Application B | Delta B | Free B | Disposition |
| --- | ---: | ---: | ---: | --- |
| Baseline .58 | 583,388 | 0 | 948 | Exact baseline reproduced |
| 1. Shared analog body and filter state calculation (`analog-tap`) | **581,532** | **-1,856** | **2,804** | Best conditional candidate; hardware timing/stack gate |
| 2. Shared body, direct dispatcher (`analog-direct`) | 581,596 | -1,792 | 2,740 | Slightly simpler fallback; same hardware gate |
| Diagnostic: shared body, original wrappers | 581,628 | -1,760 | 2,708 | Superseded by direct dispatcher |
| Shared out-of-line soft knee | 583,372 | -16 | 964 | Reject: negligible saving, per-sample call added |
| Delay target prepared per block | 583,436 | +48 | 900 | Reject as headroom work |
| LOFI stereo control expressions hoisted | 583,452 | +64 | 884 | Reject as headroom work |

All target placement and region checks pass. All variants have main static RAM **86,176/98,304 B**, low RAM **28,928/29,496 B**, pool **332,876/344,064 B** with the existing 8,192-byte reserve. Thus RAM deltas are **0 / 0 / 0 B**. The low region retains only **568 B**. No new voice, audio buffer, heap allocation or lookup table is introduced.

The final candidate's entire image reduction is in loaded `.text`: **580,072 -> 578,216 B**. `.data` remains 300 B, `.bss` 85,872 B, `.ram_text` 2,956 B, `.c1_text` 60 B and `.noinit` 15,464 B. Main region extent includes its original alignment gap. NOLOAD regions consume RAM, not image payload. These are linked application bytes, not object-file lengths or padded package size.

## Candidate 1: share the analog oscillator/filter body

Patch: `build/headroom-ambient/dsp/analog-tap.patch`; candidate source: `tap-source/firmware/src/eng_analog.c`.

The baseline explicitly forces `analog_body` inline twice: one complete oscillator/noise/drive/filter/output loop specialized for LP, and a second for BP/HP. Their emitted functions occupy **1,830 B (`analog_render`) + 1,878 B (`analog_render_f`) = 3,708 B**. The candidate retains one noinline block renderer and a small dispatcher: **1,818 B (`analog_body`) + 34 B (`analog_render`) = 1,852 B**. Their difference exactly accounts for the **1,856-byte linked image reduction**; this is not an estimated duplicated-source saving.

The dispatcher maps filter modes exactly as before: zero -> LP, one -> BP, every other nonzero value -> HP. There is one call per voice/block, not one per sample. The original `analog_tap` and `tsvf_lp` compute the same `v1`, `v2` and clamped state updates. Returning `v2` for ft=0 allows those updates to share one source path; BP/HP retain the same formulas and rounding. Oscillator phase, noise draw order, drive, saturation, modulation, amplitude scaling and all state writeback are unchanged. No generic callback or runtime interpreter is inserted in the sample loop.

The raw patch intentionally preserves surrounding source/comments for review; integration should update the obsolete comment describing two specialized copies. No preset initializer, engine index, protocol ID, saved layout or upstream-facing header changes.

### Correctness and cost evidence

- Both direct and final shared-tap candidates pass **20,000 differential cases** against a separately named copy of the original analog implementation in the same host executable. Every sample in the complete CTL output buffer and every byte of complete voice state match. Cases cover all five waveforms, all three filter outputs plus noncanonical nonzero mode mapping, block lengths 0..32, random parameters, phase/filter/noise states, pitch, positive/negative fine factors, amplitude ramps and additive destination contents. `host.py` preserves the oracle and assertions.
- Both candidates pass **200 unchanged golden renders**, zero health failures, zero voice/routing failures, zero CPU entries over budget and zero crashes. Final summary: `tap-host-regress.log`. CPU figures are host diagnostics, not target timing.
- Final candidate passes existing `lofi_sampler_test`, `lofi_character_test` and `fx251_test` with assertions enabled. This retains loud internal-bus sampler gain, exact bypass, stereo, transitions, silence and legacy FX behavior. Host compilation uses pinned Zig 0.13, `-O2 -UNDEBUG`, private caches and private outputs. An initial direct-candidate LOFI run lacked its required `build/lofi-master/` output directory; the rerun passes after creating that private directory. The final candidate's directory is prepared before running.
- Target `build.check` passes, including **925 RAM-code instructions with no calls**, unchanged 60-byte core-1 loop and all memory guards. The final repeated run passes all existing target budget records without changing budgets. The initial shared-tap budget invocation could not find tests in its newly prepared tree; after copying the frozen tests the repeat runs and passes, recorded in `analog-tap-repeat/budgets.txt`.
- The stock checker monitors `analog_render`, whose loop moved, so its apparent cost collapse is not a speed claim. `costs.py` explicitly adds `analog_body` and `analog_render_f`, also recording mix/FX helpers when emitted; `extended-budgets.json` and `budget-deltas.json` retain the comparisons. All other monitored functions' metrics are identical.

| Actual loop owner | Loop instructions | Static divides | Loop calls | Weighted estimate |
| --- | ---: | ---: | ---: | ---: |
| Baseline LP `analog_render` | 474 | 18 | 0 | 618 |
| Baseline BP/HP `analog_render_f` | 484 | 18 | 0 | 949 |
| Shared body, separate LP expression | 480 | 20 | 0 | 640 |
| Final shared-tap body | 486 | 20 | 0 | 646 |

The final shared body's weighted estimate is **4.5% above baseline LP**; the direct fallback is **3.6% above LP**. These are static disassembly metrics over branch/loop spans, not executed instructions or cycle counts for a particular waveform. The two additional counted divides must not be described as exactly two additional dynamic divisions per sample; paths differ and coefficient work can intersect the checker's loop spans. Likewise the lower BP/HP estimate is not evidence of a hardware speedup. Neither candidate adds a call inside its own sample loop. A call at the voice/block boundary and changed register/stack allocation still require deadline and stack measurements.

**Upstream conflict burden:** low to moderate, concentrated in one short helper and render dispatcher in `eng_analog.c`; future oscillator/filter changes must be applied once. Upstream's dedicated LP specialization is an intentional performance policy, so this should be an explicit size/performance decision, not an unconditional cleanup. No ABI, compiler-wide flag or project compatibility change is needed.

## Rejected experiments and remaining scope findings

**Soft-knee outlining:** `soft_knee` currently serves NOISE, rather than a large pool of retained duplicate kernels. Forcing it out of line saves only 16 B while changing `noise_render` from zero to one static loop call. Its apparent local weighted estimate falls 243 -> 207 because work moved to a callee. Reject; do not interpret that as a hot-loop win or combine it with the analog candidate.

**Delay control target:** moved the original mode/drive target calculation from `delay_character_sample` to the block boundary while retaining per-sample mode transition/fade behavior. Net image grows 48 B. No linked saving, dynamic control equivalence or target timing improvement is claimed; the changed read timing would also need an ownership check before adoption. Preserved prototype: `delay-control.patch`.

**LOFI shared control expressions:** hoisted same-sample stereo-invariant radio/band, low-pass and tape/VHS damage coefficients outside the two-channel loop. Net image grows 64 B; unconditional damage calculations also run for irrelevant modes. Reject this version. The nested TAPE-mode check is source-redundant inside the already guarded TAPE branch, but constant reasoning removes the redundant test; deleting its source text is not evidence of additional linked savings. Sampler coefficients remain block-rate and above-unity gain remains the .58 integer-plus-fraction implementation. Preserved prototype: `vinyl-control.patch`.

**Reverb/delay variants:** STEREO/ROOM/PLATE/HALL/DARK are parameter rows in `REV_TUNE` feeding one diffuser/FDN kernel, not five copies. SPRING is a different dispersive network with independent state and a shared storage union. Merging it with the FDN or narrowing/reordering its rounded arithmetic would change the sound or risk range regressions; no such alternative is recommended as equivalent. Existing FREEZE widening and limiter wide products are correctness protections. Input is a wide internal bus, not guaranteed int16; no safe, broadly useful replacement by 32-bit products was established.

**Voice/modulation and other engines:** live vibrato/tremolo/wah/pan already share state shape and waveform advancement; their different signal effects remain necessary. LFO/tempo coefficient helpers already isolate divisions at control rate. TRIO already consumes `m->fine`, so the old review's reconstruction-removal opportunity is spent. Engine callbacks in descriptors, alternate compiled paths and SLICE-disabled source must not be called dead solely because textual direct-call searches find no use. No large removable live DSP path was established in this audit. Further drum insert coefficient sharing remains unmeasured and must respect each channel's independently fading type/state; it is not a promised saving.

No new sample compression/decoding, startup codec investigation, hand assembly, font optimization, sound-changing approximation or feature removal was attempted.

## Concrete handoff

Use **`analog-tap.patch` as the primary reviewable prototype**, with `analog-direct.patch` available if the extra 64 B is not worth the filter helper change. They are alternatives. Before integration, extend the production target budget coverage to follow the actual `analog_body` loop, retain the original LP reference comparison, and update the stale specialization comment. Repeat a same-version target link and the differential/golden tests after combining any other specialist changes; do not add independent savings arithmetically.

Hardware acceptance must exercise maximum analog polyphony and oscillator/drive/filter extremes alongside heavy shared FX, USB full-level recording and core splitting; measure worst-case audio deadlines and stack high-water. Host hashes and static budgets do not establish those results. Until that evidence exists, this report recommends the candidate for staged validation, **not unconditional replacement of the specialized LP path**. No device test, publication, version bump or production integration is claimed.
