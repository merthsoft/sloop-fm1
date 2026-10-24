# Ambient headroom: C and pi32v2 assembly audit

2026-10-24. Audit only, against the actual dirty 2.8.0.58 working sources. Production firmware is unchanged: every firmware input still matches the initial SHA-256 manifest. No version bump, shared asset regeneration, shared build output, installer, package, Git mutation, or hardware flash was performed. All experiments are under `build/headroom-ambient/asm/`.

## Recommendation

Start with the contract-preserving **C constant-bound clamp specialization: 544 linked flash bytes saved**. A bounded, portable pi32v2 inline-assembly version saves **1,056 bytes**, only **512 bytes more than C**. Keep that assembly as an optional hardware-gated follow-up, not a speed claim. Neither result establishes multi-KiB ambient/drone headroom. RAM recovery from these candidates is **zero**.

There is no case here for rewriting engines, filters, the sampler, or whole subsystems in assembly. The compiler already emits native signed min/max, register-pair multiplies and multiply-accumulates. New out-of-line calls for tiny per-sample primitives are not recommended. A fresh object inventory found no large obvious dead/duplicate code pool.

| Rank | Candidate | Target image B | Saved B | Free / 584,336 B | Disposition / upstream burden |
|---|---|---:|---:|---:|---|
| Baseline | Current .58, repeated identically | 583,388 | — | 948 | Frozen input |
| 1 | C clamp, only proven ordered constant bounds | **582,844** | **544** | **1,492** | Preferred first step; two small source edits, caller-visible macro boundary |
| 2 | Portable inline ASM clamp, same bound restriction | **582,332** | **1,056** | **2,004** | Optional; +512 B over C; same two edits plus architecture guard/constraints and hardware gate |
| 3 | Signed-wide C Q16 multiply | 583,340 | 48 | 996 | Defer: tiny flash gain, coefficient contract must be explicit; no ASM advantage |
| Diagnostic | Unrestricted C min/max clamp | 582,796 | 592 | 1,540 | Not the handoff: changes inverted-bound semantics |
| Diagnostic | Unrestricted C upper-first clamp | 583,132 | 256 | 1,204 | Rejected in favor of better C and preserved contract |
| Diagnostic | Unrestricted ASM, correct early-clobber | 582,092 | 1,296 | 2,244 | Not the handoff: changes inverted-bound semantics |
| Rejected | Constant check inside inline function | 583,388 | 0 | 948 | Pinned compiler does not specialize this form |

These are complete target links, not object estimates. The candidates are alternatives; do not add their savings. The Q16 candidate was measured independently, not combined with clamping.

All linked variants have main static RAM **86,176/98,304**, low RAM **28,928/29,496**, pool **332,876/344,064**, `.data` 300 B, `.bss` 85,872 B, `.ram_text` 2,956 B, `.c1_text` 60 B. The pool keeps its 8,192-byte reserve. Baseline `.text` is 580,072 B; final C is 579,528 B; final ASM is 579,016 B. Flash deltas are entirely `.text`; static RAM invariance does **not** mean stack usage is unchanged.

## Inputs, toolchain and reproduction

Read the prior architectural, optimization, compiler-flags, redundancy, link, storage, assets, UI, piano-roll and loudness audits. Preset/table restoration, startup/BSS work, scratch recovery, font/trim changes, UI consolidation and existing decoder specialization were treated as integrated history, not new savings. No runtime sample compression/decoding scheme was introduced. Fonts were not an optimization focus.

`source/` was copied directly from working `firmware/`, `tools/` and existing `build/gen/`, never from HEAD. `inputs-sha256.json` records those inputs. Tests were subsequently copied into private host trees. `production-firmware-comparison.json` is an empty difference list. Only scratch `dsp.c` and `ui.c` differ from the original source snapshot; generated inputs remain identical. The parent committing its existing .58 work did not change this baseline.

Compiler: existing WSL Ubuntu JieLi `jieli-linux-toolchains-20250324.1`, reporting Clang 4.0.1, direct `-target pi32v2`. Executable hashes are in `toolchain-sha256.json`; predefined target macro is verified as `__PI32V2__`. No toolchain installation or update was performed. Flags:

```text
-target pi32v2 -Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function
-DFELUCCA_BOOT_TABLES=1 -DFELUCCA_SLICE=0 -DFELUCCA_ID="FM-1_900"
-Ifirmware/hal -Ifirmware/src -Ibuild/gen
```

Normal two-pass preset probing/packing runs **only inside the snapshot**. Final C compilation adds `-DFELUCCA_BOOT_PRESETS=1`. No version override; all other feature overrides unset. Assembly uses `-target pi32v2 -c`; the unchanged linker script and standard five application objects are used. The real 584,336-byte slot is enforced; no enlarged XIP experiment is needed. `run.py` calls only private `build_app()` and `check()`, not package/loader generation. Final directories retain exact command arrays, source, ELF/object, load payloads, disassembly, symbols, section sizes, safety checks and budgets. Early trials use the same driver/flags; exact command logging was added for later/final trials.

From the scratch directory under WSL, `python3 repeat.py` repeats baseline and the bounded ASM measurement; `python3 final-targets.py` measures final signed-bound C/ASM forms; `python3 checks.py` checks every saved target. `run.py` builds the current **scratch** source, so select the intended saved `dsp.c`/`ui.c` first. Do not run a shared normal build to reproduce these experiments.

Baseline repeat SHA-256:
`f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`.
Final C and its prior bounded formulation are byte-identical:
`401b0bcc6b83e373012991d0b8a9b33e58b4fd21eb47f91c46c56e5917323d52`.
Bounded ASM repeat, signed-bound fix and portable architecture guard all produce the same image:
`231070b73c384a1f365490b07672b5fab3afff868e23e3d66bbd42eca3436096`.
See `final-size-hashes.json`.

## 1. Clamp specialization: the useful small kernel

Current `dsp.c:14` uses `v < lo ? lo : v > hi ? hi : v`. A standalone target leaf occupies 12 B: conditional signed comparison, `smin`, move, return. Ordered clamping written as two C assignments emits `smax`, `smin`, return: **10 B**. Inline assembly and out-of-line assembly produce exactly the same standalone bytes as that simpler C. Full-image differences arise from how each form is optimized and allocated at its callers, not a unique min/max instruction unavailable to C.

Replacing every clamp silently changes a real part of its function contract: `clamp(0,10,-10)` currently returns 10; lower-then-upper min/max returns -10. Successful existing tests are not proof no future/damaged-input call can invert bounds. Therefore final prototypes retain the original function and specialize only **caller-visible, compile-time, ordered int32 bounds**. The macro selects the original function otherwise, evaluates the value once, and compares bound constants after the same int32 conversion as the old function. This also preserves `clamp(0,0,UINT32_MAX) == -1`.

The pinned compiler folds `__builtin_constant_p` inside the always-inline helper to false in this experiment (zero saving). A caller-visible macro works. Existing `ui.c` already rebinds `clamp` to its shared cold `ui_clamp`; the prototype adds `#undef clamp` immediately before that existing rebind, keeping its scope and later original function available. An initial scratch macro trial omitted the original function name and failed to link after the UI undef; that failed form was corrected and is not a handoff patch.

Reviewable alternatives:

- `build/headroom-ambient/asm/clamp-c-final.patch`: C implementation, 544 B saving.
- `build/headroom-ambient/asm/clamp-asm-final.patch`: final portable ASM implementation, 1,056 B saving. `clamp-asm-portable/` holds its actual target output.

The assembly body is just:

```c
__asm__("%0 = smax(%0, %1)\n\t%0 = smin(%0, %2)"
        : "+&r"(v) : "r"(lo), "r"(hi));
```

The early-clobber `&` is required because the first instruction writes `v` before the second consumes `hi`. A preliminary `+r` trial has equal size but different bytes; it is explicitly rejected. Do not copy it. The final expression is nonvolatile: unused pure results can be removed. It has no memory access, shared scratch, interrupt state, fixed register, stack operation or hidden callable helper. Comparisons name their operands directly; no compiler-visible condition-code result is consumed. Only the declared output is changed. It is reentrant across both cores. The `__PI32V2__` branch has a C fallback for host/upstream builds; portability was actually compiled and tested.

The bounded ASM shrinks `mix_block` by 208 B, `formant_render` by 106, `px_kill_run` by 84, `ins_swept` by 80, `ins_drive` by 70 and `slicer_track` by 62. These are explanatory symbol deltas, not extra savings to add to the 1,056-byte link delta.

### Stack and performance limits

Neither candidate is a demonstrated speed improvement. `final-stack-inventory.json` records these exact static disassembly observations:

| Function | Baseline local frame / textual SP references | C candidate | Bounded ASM |
|---|---:|---:|---:|
| `mix_block` | 188 B / 454 | 204 B / 465 | 204 B / 505 |
| `events_block` | 396 B / 225 | 396 B / 224 | 396 B / 194 |
| `formant_render` | 372 B / 195 | 380 B / 187 | 372 B / 174 |

Frames exclude the separate saved-register area; SP-reference counts include saves/loads/addressing and are **not** dynamic spill counts or cycle counts. In particular, smaller flash adds static stack traffic in the large mixer. Broad assembly rewrites based on a smaller instruction count would be unjustified. Check hardware worst-case audio deadlines and both cores' stack high-water before accepting either change.

Unchanged target budgets pass. For example, bounded ASM changes `formant_render`'s monitored loop count 313→296, weighted estimate 911→849; `slicer_track` 296→284, estimate 1077→1038; `digital_render` increases 339→340. All keep their prior loop divide/call counts in these examples. C changes formant loop count 313→319 while its weighted estimate falls 911→896. These static diagnostics are not target cycle measurements. Every final placement check retains 925 call-free `.ram_text` instructions, the 60-byte core-1 loop, scratch containment, ROM-reference rules and pool reserve.

## 2. Q16: C is already better than hand assembly

The current split `mulq16` leaf is 14 B. A full mixed signed/unsigned `(int64_t)a * k` leaf is **22 B**, introduces sign correction and saves/restores `r4`; that seemingly simpler spelling is worse. Casting the bounded coefficient to signed before the wide multiply emits:

```text
r1_r0 = r1 * r0 (s)
r1_r0 >>= 16
rts
```

That C leaf is **10 B**, exactly matching the assembled out-of-line leaf. A naive inline-ASM pair-output probe is 12 B due to an extra register move. Independent `r` outputs do not establish a general allocator-safe adjacent register-pair constraint; that probe is diagnostic, not recommended inline-ASM source. The full C change saves only **48 image bytes**; `mix_block` actually grows 12 B. No helper runtime library or 64-bit multiply call is added. This is a good example of why leaf-byte savings cannot be multiplied by source call count.

Reference comparisons cover all 65,536 coefficients 0..65535 for 15 signed extrema (983,040 cases) plus one million random pairs. That domain matches the split formula exactly. A blanket general-Q16 equivalence claim would be false: `a=65535, k=65538` yields split 0 versus wide 65536 because the split low product wraps. Coefficients outside the proven range need a separate contract/range audit. The wide result's final int32 narrowing is also only meaningful within the caller's intended representable range.

**Do not replace the .58 sampler gain decomposition in `vinyl.c:109` with the split helper on an above-unity coefficient.** This audit leaves that corrected expression untouched. The real sampler test passes in baseline, clamp, Q16 and portable-fallback builds, including mild 20% DRIVE, loud internal bus, signed quantization and stereo isolation. A full high-coefficient split rewrite is rejected regardless of its flash size.

## Other emitted idioms and rejected directions

`kernels.{c,s,dis,o}`, `kernels-out.{S,o,dis}` and `kernel-equivalence.json` compare actual target bytes; only their standalone sizes are object measurements:

| Leaf | Emitted bytes | Finding |
|---|---:|---|
| Q15 multiply/shift | 6 | Native low multiply, arithmetic shift, return; no helper call. Replacing it with a rounded/saturating/full-product DSP instruction changes overflow/truncation semantics. |
| xorshift RNG | 14 | Three shifts/XORs, return; ordinary C is already a bounded leaf. No shared RNG scratch should be introduced. |
| Ordered clamp | C 10 / inline ASM 10 / out-of-line ASM 10 | No intrinsic advantage demonstrated; C already selects real ISA min/max. |
| Signed Q16 | C 10 / inline probe 12 / out-of-line ASM 10 | Prefer C; preserve coefficient bounds and signed rounding. |
| Existing amplitude ramp | 18 | Explicit bias+shift preserves division toward zero for negative ramps. Simple `/32` is 14 B but emits a hardware divide; reject per-sample divide just for four leaf bytes. |
| Three-product rounded Q30 sum | 32 | Compiler already emits three signed register-pair multiply-accumulates. Six-argument ABI loads two arguments from stack and saves `r4..r7`; hand assembly does not eliminate that ABI. |

No documented min/max intrinsic was found in the installed toolchain resources inspected; no guessed builtin was adopted. Existing C lowering supplies the relevant native operations. The real formant output contains signed multiply-accumulates already; it is not calling a software 64-bit multiplication helper. Large stack frames in formant/mixer are reasons for carefully bounded C lifetime work, not permission to fuse saturated, rounded filter stages or discard intermediate limits.

Out-of-line leaves use the observed ABI: `r0..r3` first arguments, `r0` scalar result, later arguments on stack; generated callees preserve used `r4+` registers and `rets` as needed. Scratch assembly uses two-byte instruction alignment. A direct `call` is four bytes and `rts` two, before argument moves/caller preservation. An inline 8-byte two-operation clamp becomes a 4-byte call to a 10-byte reusable body; sufficient sites can reduce flash, but add per-sample calls and register constraints. No full-image savings or cycle claim is made for an out-of-line production rewrite; it is rejected as a preferred audio-path trade. Existing cold UI outlining remains intact.

## Fresh code-bloat/dead-code check

`code-inventory.py` examines all **910 nonempty emitted C functions** in the current baseline object, including relocation identity when comparing bytes. Exactly two duplicate groups remain: `dstep_mask` / `ota_rd16` (10 B each) and `fm1__wait` / `fm1_delay_us` (22 B each). Their gross removable-body ceiling is only 32 B before call/layout effects, and the input idle hook and hot/cold ownership still matter. These confirm the earlier link audit rather than creating a new saving.

The five function sections without incoming C-object section relocations are `fm1_alnk0_irq`, `fm1_c1_main`, `fm1_cstart`, `fm1_fault_c` and `fm1_timer5_irq`. Every one has a direct assembly reference in `fm1_isr.S`, `fm1_cpu1.S`, `crt0.S` or `fm1_vec.S`. They are not dead. This object-level check is not a proof that all semantic redundancy is exhausted; it rules out a large simple identical-body or apparently orphaned-function deletion from this snapshot. No repeated GC, LTO, vector-table removal or blanket noinline proposal is counted as new work.

## Correctness evidence and concrete handoff

All four private host configurations—baseline, unrestricted ordered C with `lo<=hi` assertions, signed-wide Q16 C, and the **actual portable ASM patch's C fallback**—pass:

- **200 unchanged golden renders**, zero health, voice/routing failures or crashes; host CPU diagnostics pass but are not target performance evidence.
- Real sampler mechanism/loud-bus regression checks.
- Actual UI suite including **20,000-frame fuzz**, then separate **20,000-frame stress** with clean stop.
- 79 UI captures per baseline/C/Q16 run, all byte-identical; the portable run's captures were also compared with baseline.

The final portable macro additionally passes **2,003,375 actual native-C comparisons** over extrema, arbitrary/inverted random bounds and constant-bound specialization, plus single evaluation and unsigned-to-int32 bound conversion checks (`portable-reference.c`, `.log`). Independent Python checks add 1,001,800 ordered clamp comparisons and the Q16 comparisons above. Target min/max leaf bytes match compiler-generated C exactly. **No target emulator or physical execution of the new assembly was performed**; host fallback tests and emitted-byte equivalence do not substitute for that gate. No hardware cycles, cache behavior, worst-case interrupt latency or stack high-water were measured.

For integration, inspect **one** final patch, retain its caller-visible specialization, original dynamic/inverted-bound fallback, int32 bound casts and the UI undef boundary, then repeat the same-version combined target build and behavioral checks with other specialists' changes. Prefer C for the first review. If the extra 512 B warrants ASM, retain `__PI32V2__`, the host C fallback and `+&r`, and require actual hardware audio/stack validation before release. Do not combine the unrestricted diagnostic clamp or Q16 changes merely to maximize a spreadsheet saving. `clamp-asm-final.patch` is a concrete reviewable prototype; no production integration is authorized or performed by this audit.
