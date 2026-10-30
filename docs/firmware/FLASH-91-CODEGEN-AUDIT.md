# .91 target code-generation and linker audit

2026-10-28. The useful result is a **320-byte application-flash candidate**, with **zero static RAM cost**. The preferred review patch limits threshold-zero inlining to application compilation and explicitly retains inline timer/interrupt primitives. It is a hardware-gated candidate, not an adopted production setting or a demonstrated speed improvement. No multi-KiB code-generation win was established.

## Baseline and isolation

The input is the actual dirty **2.8.0.91** snapshot at `build/flash-audit-91/baseline/source`, including hold-delay and worker-hardening changes, not Git HEAD. All experiments, source copies, patches, commands, compiler outputs, oracles and logs are under `build/flash-audit-91/codegen/`. Only this uniquely named report was written outside that private directory. Production firmware/assets, shared outputs, installer, versions, existing reports and Git state were not changed. No flashing, commits, pushes, PRs or delegation occurred.

`base/` reproduces **584,296 B** and is byte-identical to the frozen baseline:
`c98a63741a9f16f2b9f7faa753dd31c980241a7013e56121339fbb03274552ad`.
`verify.py` checks all **346** manifest inputs in the original frozen snapshot; `snapshot-verification.json` records zero differences. Generated inputs and the three pinned SDK files were copied, not regenerated from production. Target preset/literal intermediate generation is confined to each private source tree.

Read prior architectural, engine-integration, comparison, literal-storage, optimization, compiler-flags, duplicate-code, linker and ambient-ASM audits. Already integrated preset/table/name packing, native clamp specialization, dynamic coefficient sharing, shared analog rendering and drum priming are historical savings, not available new gains. In particular, .90's six MODAL/MEMB clamp changes are retained; the rejected experiment below concerns the remaining STRING/SYMP sites.

Pinned compiler: WSL Ubuntu, `JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, vendor Clang 4.0.1, pi32v2. All runs explicitly set **FELUCCA_AUDIO_DIAG=0**, retain source version, normal features, -Oz, upstream formats, all checks and the actual **584,336-byte** slot. No linker region enlargement or toolchain replacement was used. Target builds ran sequentially. No host timing or shared-host throughput is presented as device timing.

Initial reproduction uses the supplied frozen `build/build-target.py`, including its private loader/package. Subsequent candidate driver `run.py` invokes only `build_app()` and `check()`, never the oversized-image logo fallback or production packaging. Every successful candidate retains symbols, sections, ELF, binary, disassembly, map/cross-references, exact command arrays and unmodified target-budget results in its own `build/` directory. Failure JSON/logs preserve compiler or linker diagnostics.

## Ranked measured results

All completed links below use **97,500 / 98,304 B main**, **28,928 / 29,392 B low**, and **335,424 / 344,064 B pool**. Thus main free is 804 B, low free 464 B, and pool gap is 8,640 B, only **448 B beyond the retained 8,192-byte reserve**. No candidate spends reserved RAM. Loaded .ram_text is 2,956 B, .c1_text is **160 B**, .data is 384 B; .bss is 97,116 B. These current low-RAM/core-1 figures supersede older reports' smaller core-1 stub and larger low capacity.

| Rank / experiment | Linked app B | Saved vs .91 B | Flash free B | Disposition |
|---|---:|---:|---:|---|
| Frozen .91 / reproduced base | 584296 | 0 | 40 | Baseline, byte-identical |
| 1. App threshold 0 + always-inline timer/IRQ primitives (`scoped0`) | 583976 | **320** | **360** | Preferred review candidate; hardware gates remain |
| Threshold 0 + same inline primitives, global CFLAGS (`hardened0`) | 583976 | 320 | 360 | Repeated identically; superseded by app-scoped patch |
| Threshold 0 alone (`inline0`) | 583976 | 320 | 360 | Repeated identically; less desirable ISR call boundaries |
| Threshold 0 + remaining PHYS native clamps (`combined`) | 584008 | 288 | 328 | Actual combined link; reject PHYS addition |
| 2. `-fdata-sections --gc-sections` (`data`) | 584292 | 4 | 44 | Insignificant; no function-body saving |
| `--gc-sections` (`gc`) | 584296 | 0 | 40 | Byte-identical baseline; no dead-section windfall |
| `-mllvm -inline-threshold=5` (`size`) | 584296 | 0 | 40 | Byte-identical baseline |
| Remaining PHYS native clamps alone (`physclamp`) | 584328 | **-32** | 8 | Reject: grows flash and STRING loop |
| `-mllvm -enable-global-merge=false` | No final image | — | — | Supported option, link overflows XIP by 780 B |
| `-fno-jump-tables` | No final image | — | — | Link overflows XIP by 392 B |
| `-flto` | No final image | — | — | Preset packer rejects bitcode: expected little-endian ELF32 |

Alternatives are not additive. The PHYS/threshold combination was actually linked; its 288-byte saving is not a sum substituted for a build. Linker overflow reports above describe failed section placement, **not** exact full-image sizes or valid RAM measurements for those failed variants.

## Exact patches and application order

Preferred single patch: **`build/flash-audit-91/codegen/scoped0.patch`**. Apply it once to the frozen baseline; do not first apply the alternative flag patches. It changes only:

1. `tools/build.py`: add `-mllvm -inline-threshold=0` to the application-local `flags` in `build_app()`, retaining ordinary global CFLAGS for the loader and generated literal blob.
2. `firmware/hal/fm1_time.h`: make `fm1_ticks` explicitly always_inline.
3. `firmware/hal/fm1_flash.h`: make `irq_save` and `irq_restore` explicitly always_inline, retaining their exact volatile assembly and memory clobbers.

The latter two edits prevent threshold-zero outlining of the low-level hooks. The patch does not add assembly instructions or change a register contract. All three edits are one measured candidate; the inline-attribute changes are not independent flash savings.

Diagnostic alternatives retained for review: `inline0.patch`, `hardened0.patch` and rejected `physclamp.patch`. The measured `combined/` source is exactly `physclamp/` plus the threshold-zero option injected by `run.py`; its `build/commands.json` records that combination. Reproduce it by selecting `combined`, not by summing rows. These patches should not all be applied together. No production patch was applied.

## Runtime, stack and ownership of the preferred candidate

The option is a supported internal LLVM option in this pinned distribution, not an unsupported function-merging flag. It changes ordinary compiler inlining decisions while preserving explicit DSP always_inline boundaries. It is compiler-specific and would require remeasurement for a compiler or source change.

All monitored engine/ISR instruction, loop, divide, call and weighted-cost results are identical to baseline for the inline-hook candidate; `code-deltas.json` records no changed budget metrics. This is stronger than merely passing the tolerance. It is **not** whole-program runtime equivalence or a speed claim: the unmonitored mixer/event setup does acquire calls.

Static disassembly observations (`stack-summary.json`, `guarded-call-frame-deltas.json`):

| Function | Baseline local frame B | Candidate frame B | Direct call sites before -> after | Consequence |
|---|---:|---:|---:|---|
| events_block | 396 | 392 | 194 -> 201 | Seven outlined trk_index call sites |
| mix_block | 220 | 220 | 53 -> 56 | Three outlined cpu1_ticks call sites |
| fm1_alnk0_irq | 16 | 16 | 5 -> 5 | Inline timer/IRQ primitives preserve these boundaries |
| fm1_timer5_irq | 20 | 20 | 12 -> 12 | Same direct call-site count and local frame |
| seq_step_velocity | 20 | 16 | See full inventory | Smaller local allocation, not a stack-watermark result |

Frames exclude saved registers and transitive callees. The listed large functions preserve their saved `{rets,r15-r4}` set. For the candidate, textual SP references remain 381 in mix_block, 14 in each listed ISR; events_block falls 195 -> 190. These are static instruction-site counts, not dynamic spill counts, execution counts or cycle measurements. No tracked common function increases its explicit entry local allocation in the full call/frame inventory, but new calls add return-address/save work and can increase transitive stack depth.

The unguarded threshold option instead changes the audio ISR from 205 to 203 local instructions while retaining loop 53/divides 0/calls 2/cost 137, yet increases total direct call sites 5 -> 7; timer ISR call sites increase 12 -> 16. This motivates the preferred guarded patch despite identical flash size.

No persistent object, buffer, memory arena, core ownership, RNG, synchronization protocol, worker timeout, overload threshold or factory sample changes. `cpu1_ticks` still performs the same volatile timer read, but its additional call cost belongs in the worker/join/deadline hardware test. `trk_index` still reads the same track identity; seven call sites do not imply seven calls every block. No speculative target speed benefit is claimed from reduced instruction footprint.

## Reachability, switch lowering and emitted duplication

The .91 .text output (code and constants together) is **580,796 B**. Named emitted .text functions total **238,808 B**, while unchanged SMP_DATA alone is **303,054 B**. Neither symbol sum is a deletion budget; merged aliases and other data require independent accounting. Largest functions include mix_block 13,148 B, events_block 9,408 B, layer_screen_draw 5,944 B, ed_handle 4,300 B and mix_part_run 3,920 B (`largest.json`). Large size identifies an investigation boundary, not removable or safely interchangeable work.

Current full linker GC produces the same binary hash and discards only zero-length default .text inputs in its map. It does not expose an unused multi-KiB function/asset pool. Adding data sections and GC leaves every function size unchanged and all RAM extents unchanged; its four bytes are layout/constant effects, not evidence of erased code. Loaded map padding is 12 B (2 B before vectors, 8 B before C rodata, 2 B at .ram_text end); another 2 B aligns the NOLOAD low-RAM start and is not image headroom. Alignment must remain valid.

The linked function-body scan finds exactly the same tiny duplicate pairs as the old audit: dstep_mask/ota_rd16 (10 B each), fm1__wait/fm1_delay_us (22 B each). Their **32 B gross duplicate-body bound is not net savings**. Cold OTA and playback should not gain a new shared audio-path dependency for ten bytes; the input wait also retains its build/test hook contract. This scan compares final loaded bytes; it does not prove all semantically related or differently relocated helpers unique.

`switches.json` records 18 matched compiler jump-table start/end label pairs spanning 498 B. It is not a full switch inventory or removable-data claim; other lowering forms and branch code are separate. The current blanket no-jump-table experiment fails the real link. Earlier source also regressed DIGITAL's loop budget with this flag; that historical failure is not falsely reported as a fresh .91 linked budget. No switch feature/default checks were removed.

Larger opportunities remain source restructuring of cold UI/control families, where the inventory confirms several KiB bodies. That is an **unmeasured** architectural project; the earlier 4-10 KiB planning estimate is not validated here. It needs one end-to-end view/handler conversion with framebuffer, event, storage and target-link oracles. Neither compiler flags, symbol stripping, section GC nor the duplicate-byte scan established a multi-KiB alternative. No executable overlay or larger partition proposal is included.

## LTO and assembly: supported capability versus current pipeline

The prior compiler audit correctly established that this compiler supports LTO **through vendor lto-wrapper/LLVMgold**, saving only 32 B on its older source. Plain ld rejection and the unknown enable-merge-functions option do not mean LTO is unavailable. Replacing the compiler was unnecessary in that audit and is excluded here.

Today the build has an additional constraint: preset probing and literal relocation inspect ordinary ELF32 objects. The .91 -flto probe successfully compiles but then `pack_presets` rejects bitcode before any LTO final link. Thus this audit has **no .91 LTO size measurement**, and does not reuse the older 32 B as a current claim. A legal future experiment would need a proven ordinary-object metadata probe plus a supported LTO code-generation stage that exposes equivalent relocatable symbols for literal pooling, with full object/address/bootstrap oracles. Simply selecting lto-wrapper cannot fix the earlier packer failure. Such pipeline engineering is deferred, unmeasured and not an integration patch.

The PHYS trial reuses the already maintained fm1_clamp_ordered primitive at six remaining constant ordered sites, retaining a fallback for dynamic/inverted converted int32 bounds. That primitive uses signed smax/smin with `+&r`: early-clobber prevents the first write aliasing the still-needed high operand. It has no fixed registers, hidden state, memory effects or calls. Existing host fallback remains unchanged. Despite that sound local contract, the **full link grows 32 B**: px_string_block +36, px_string_run +28, px_symp_run +2, px_string_excite -26 and phys_render -2 function bytes. Alignment supplies the remaining difference. STRING's monitored loop goes 260 -> 266, SYMP weighted cost 255 -> 259, while exciter loop falls 165 -> 161. Local STRING frame stays 100 B but SP references grow 66 -> 71. All budgets pass, yet the candidate is rejected on net size and work distribution. No new ASM implementation, speed win or hardware qualification is claimed. No generic 64-bit multiply/ABI trick or engine rewrite is proposed.

## Validation, reproduction and limits

- Exact baseline byte reproduction, all 346 frozen hashes unchanged.
- Every completed link passes normal placement, entry stub, ROM-reference, scratch containment, image size, pool reserve and unmodified target budgets. .ram_text stays 925 instructions with no calls; core-1 RAM code stays 160 B. No safety check/budget was edited.
- Threshold-zero repeated with SHA `09d1f1d52c3d74116eed6cba9df3326b9f7463528c85c8c26b42a515d89f7b4a`.
- Preferred app-scoped candidate is byte-identical to the repeated inline-hook build (same ELF-derived application bytes); its separate link and unchanged budget results are in scoped0.json. Inline-hook candidate repeated with SHA `ac2cf114e6af2e67817bbd12a066caa922ae74cd00f800351f38f0bb65a2e6e8`.
- Actual object/final-address literal oracle and real C cold decoder pass for threshold-zero, PHYS, their measured combination and the inline-hook candidate: **1,405 complete literal objects**, final relocated addresses, three cold restores, guard bytes and UBSan. The count includes the original object symbols before pooling; the packer summary's 1,404 transformed symbols is a different count. All maintain the original literal-payload hash `a31d82665b4828ca5b09c084806a8f80503a14eff5ba800fb270f43fb1705db9`. See each `*-literals.log`.
- Host decoder compiler uses the existing optimization-cc wrapper copied into each private tree, with private Zig caches, -O2, -UNDEBUG and UBSan. Decoder arithmetic requires no changed alias/overflow assumptions. No host audio golden claim is made for target compiler-option transformations: an ordinary host compile does not exercise pi32v2 inlining/assembly/code generation.

From WSL Ubuntu in `build/flash-audit-91/codegen`, with the pinned toolchain and FELUCCA_AUDIO_DIAG=0 exported: `python3 run.py scoped0` builds the prepared preferred copy; `python3 run.py inline0` rebuilds the independent flag alternative; `python3 run.py physclamp` and `python3 run.py combined` rebuild the rejected source experiment and actual combination. `matrix.py`/`confirm.py` document earlier sequential matrices; preparation scripts create private source copies and patches. Run `analyze.py`, `details.py`, `calls.py` for code/stack evidence, `verify.py` for frozen-input checks. `validate.py` performs the private literal/UBSan proof for its selected candidate; its required output directory is created by the driver. Prepared source trees are retained, so do not rerun preparation against them without first choosing a new private destination.

Remaining gates for any adoption: worst-case audio and keyboard/ISR latency, worker completion/timeout and overload recovery under maximum voice/LOFI load, both cores' stack high-water, cold boot/recovery and normal update return behavior on physical hardware. Repeat complete final integrated target measurement if any other patch is combined. No hardware cycles, deadline margin, stack-watermark proof or portable compiler-flag guarantee is available here. Retain production settings until those gates justify this small, reviewable candidate.
