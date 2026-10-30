# Flash .99 DSP reuse audit

Production now integrates only patches 01–04, the conservative PHYS/mixer batch. The additional filter-setup and FORMANT candidates remain deferred. [Combined .100 build and new presets](CONSOLE-SOUNDS-EXPANSION.md) records actual final capacity and validation; independent savings below retain the frozen .99 baseline.

Frozen dirty MerthSloop 2.8.0.99 source, before the concurrent new-preset work. Production sources, generated output, version, installer and Git were not changed. All candidates and build products are private under `build/flash-99/dsp/`.

## Recommendation

Start with the deferred PHYS/mixer batch: **320 linked bytes saved**, reaching **583,408 / 584,336 B (928 B free)**. It was absent from .99; the historical .91 estimate of 208 B is not the current result. A fresh shared filter-setup candidate increases the measured saving to **480 B**, but has small target loop/frame growth and deserves a hardware deadline check. Neither saves KiB or changes audio quality.

Every successful matching-version link retains **97,612 / 98,304 B main static**, **28,928 / 29,392 B low RAM**, and **335,424 / 344,064 B pool**. The existing **8,192 B pool reserve** remains intact. No voice-budget change, overload-guard change, new playback decoder, sample compression or compiler flag change is involved.

## Measurements

| Candidate | Image B | Saved B | Flash free B |
| --- | ---: | ---: | ---: |
| Current .99 baseline | 583,728 | 0 | 608 |
| Shared PHYS finalization | 583,664 | 64 | 672 |
| Shared guarded PHYS dispatch | 583,696 | 32 | 640 |
| Shared limiter/output conversion | 583,568 | 160 | 768 |
| Shared accumulator clearing | 583,680 | 48 | 656 |
| **Deferred four-change batch** | **583,408** | **320** | **928** |
| Fresh TRIO/NOISE shared filter setup | 583,616 | 112 | 720 |
| Fresh drum shared filter setup | 583,680 | 48 | 656 |
| **Four-change batch + both fresh candidates** | **583,248** | **480** | **1,088** |
| Above + optional FORMANT setup helper | 583,200 | 528 | 1,136 |

Rows are complete normal target links, not summed symbol sizes. Baseline SHA-256 `9a32801a341ecf2e5d8d4ba4542a14ce008534415756ea046b3bc79b262df28b` matches the supplied image byte-for-byte. Six-change SHA-256 `c3e591aed1bb37bf4f24274a38dc0bce4a3a540d74f81cf844ec4fa879e6324c`.

## Changes and tradeoffs

The deferred batch shares MODAL/MEMB state bounds/zeroing, their common render dispatch, three mixer accumulator-clear sites, and the identical DAC/USB limiter-plus-output sequences. It retains the explicit `px_modal_run` noinline boundary, fallback clearing before rerender, independent limiter states and USB reset order. There is no new sample-loop helper call. PHYS local frame falls **924 to 700 B**; finalization adds a 12 B saved-register frame once per setup block. Mixer frame stays 228 B; the shared output helper uses 52 B register saves, versus the old limiter's 52 B saves + 4 B local. Accumulator helper has no register-save/local frame. These observations are not whole-device stack watermarks.

Fresh TRIO, NOISE and synthetic-drum setup all duplicate the existing `tsvf_coef_dynamic` interpolation/rounding. The candidate calls that existing helper once per block, preserving each caller's original damping law: TRIO uses 7168, NOISE 7800, and drums retain `fk`. It uses a local coefficient object and scalar copies; no type-punning or layout assumption is required. Source/RNG/state order and nonlinear sample kernels remain unchanged.

The setup sharing changes register allocation: TRIO frame **504 to 524 B**, NOISE **128 to 132 B**, and drum setup **0 to 12 B** local; the coefficient helper already saves 12 B registers. TRIO loop estimator **337 to 338**, NOISE **249 to 255**, with no new loop division/call; both pass original caps. This is a real structural throughput/stack tradeoff, not a claimed free speedup. Keep the fresh 160 B optional if hardware timing is sensitive. The historical optional FORMANT helper measures 48 B additional savings here but retains the known **+24 B inspected setup call-chain** stack trade; it is excluded from the preferred batch and final full-state suite.

`px_modal_run` (538 B), string render/excitation, sympathetic render, and both FM6 operator kernels are byte-identical to baseline. FORMANT loop structure remains unchanged in the six-change candidate. Factory sample bytes (303,054 B), vowel/coefficient objects, FM6 algorithms and all three membrane tuning arrays compare byte-for-byte across linked images.

## Validation and reproduction

The final six-change frozen source passes the private UBSan full-state oracle: **80,000 PHYS setup**, **60,000 FORMANT output/voice/shared-RNG**, **10,000 PHYS renderer/arena/RNG**, **100,000 accumulator-clear/limiter-output/state**, plus **60,000 each TRIO and NOISE output/full-voice and drum coefficient state** comparisons. It includes zero/short/full blocks, randomized live controls, original dispatch and sentinel output regions. Existing fixed-point shift checks are excluded with `-fno-sanitize=shift`; this is not an all-UB claim.

Assertions-enabled 200 unchanged audio goldens, arena ownership, FX251, LOFI sampler/character and FM6 suites pass. All original target placement, loop budget, 25-function inlining and master LOFI zero-loop-division/call checks pass without altered thresholds. The real mixer suite covers fourteen scenes and thirteen masks, including chaotic execution and forced lost-worker fallback. Final logs/results are under `validation-final/`; `checks-final.log` records its completion. No golden was changed.

An initial host suite overlapped private target source experiments; it is retained under `validation/` but is superseded by a full second run after final-source freeze (`final-source-manifest.json`). Oracle reference headers derive from frozen original implementations; `original-extra.h` extracts current .99 TRIO/NOISE/drum functions. `verification.json`, `hot-code-verification.json`, `tier-loops.json` and `stack-prologues.json` retain linked/data/loop/prologue evidence.

Review patches are `patches/01-finish.patch` through `06-dscoef.patch`, incremental in numerical order; stop after 04 for the preferred 320 B batch. The combined `finish-dispatchguard-limitout-accclear-svfshared-dscoef/candidate.patch` is relative to the frozen .99 source. Patches use LF context; normalize only an isolated copy when applying through GNU patch to CRLF files. `audit.py` retains each actual linked experiment and unchanged pinned SDK verification. Run private target builds sequentially through WSL Ubuntu with the existing pinned JieLi compiler, `FELUCCA_AUDIO_DIAG=0`, and normal -Oz. `oracle.py`, `host-final.py`, `checks-final.py`, and `deliver.py` reproduce evidence. No packaging/WASM/device timing/physical stack high-water qualification is claimed.

Broader FORMANT coefficient-loop alternatives, reciprocal changes and blanket master-stage outlining were not rerun: the earlier audit already found sample-span growth or failed/growing links, and the fresh filter setup is a narrower candidate. PHYS/FM6 coefficient caches remain CPU investigations rather than established flash savings; they require lifecycle/invalidation work. Already-integrated ANALOG sharing, sample decode arithmetic, DRAWBAR cache, FM6 masks, LOFI setup and .99 UI reuse are baseline and are not counted again.
