# Link, retention and initialized-state headroom audit

Audit date: 2026-10-23. No production firmware, shared generated assets, shared build outputs, Git refs, packages or served installer were changed. The sole report is this file; experiments and patches are under `build/headroom-link-51/`.

**Recommendation:** integrate the focused `punch` / `punch_filter` boot-initialization change after giving the host harnesses the same explicit initialization contract. Its frozen same-version target saves **1,488 bytes**, leaving **4,528 bytes** in the existing application slot. It adds four assignments once before either recovery or normal startup and adds no playback operation. Retain the existing compiler, DSP inlining and linker layout. Generic section garbage collection offers **zero** additional bytes on this source.

## Frozen baseline and reproducibility

The working source, including the uncommitted obsolete-icon removal and two-icon header atlas, was copied rather than exported from HEAD. `source/` contains private firmware, tools and `build/gen`; `snapshot-manifest.json` records 174 input hashes. `snapshot-verification.json` confirms all 174 remain unchanged after experiments. Private test files were copied separately and adapted to the candidate's startup contract; they are outside that original input manifest. Generated headers were copied, not regenerated; the normal target preset-probe/packer ran privately for each compile. No shared generator or target build was invoked.

Toolchain: installed `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, Clang 4.0.1, `-target pi32v2`, under WSL Ubuntu. Flags and product/version are the frozen source defaults: `-Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, boot tables/presets enabled, `FM-1_900`, unchanged source version. The private baseline exactly reproduces the requested **581,296-byte** image. SHA-256: `3354e60ca7c910b57bcc136141225f4ca294eb2dbdad6ef57048426dcb11fa13`.

The private drivers call `build.build_app()` and `build.check()`, not package creation. Linker regions were not enlarged. Each variant saves objects, ELF, disassembly, symbols, sections, relocation listing, map, commands and checks in its named directory (the initial `baseline/` has the original outputs; `map/` adds full diagnostic listings). `variant.py` chooses a private source root through `AUDIT_SOURCE`; its generated and intermediate paths remain inside that root.

From PowerShell, with this repository as cwd:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec python3 run.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec python3 matrix.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec env AUDIT_SOURCE=punch-source python3 variant.py punch
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec python3 validate.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec python3 regress.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-link-51 --exec python3 split-run.py
python build/headroom-link-51/inventory.py
python build/headroom-link-51/compare.py
python build/headroom-link-51/final-check.py
python build/headroom-link-51/evidence.py
```

The prepared private candidate/test trees are retained. Preparation scripts describe their construction. Do not run the test-preparation script repeatedly against an already-prepared tree: it inserts test initialization/instrumentation. `evidence.py` expects `source/build/felucca.elf` to be the stripped matrix output for its strip comparison; run it after `matrix.py` if rebuilding other baseline variants overwrote that intermediate. The authoritative full outputs are the named result directories. `image-sha256.json` and `validation.json` preserve final identities independently.

## Ranked measurements

All differences below are relative to the same frozen baseline. These rows are alternatives, not additive savings.

| Rank / variant | Image bytes | Delta | Main static RAM | Low RAM / pool | Disposition |
| --- | ---: | ---: | ---: | --- | --- |
| Baseline | 581,296 | 0 | 89,128 | 28,280 / 332,876 | Reproduces current source |
| 1. Both punch objects to BSS + boot assignments | **579,808** | **−1,488** | **89,112** | unchanged | Recommended focused candidate, host startup contract needed |
| Diagnostic: `punch_filter` alone | 580,076 | −1,220 | 89,128 | unchanged | Strong global-layout interaction; not separately additive |
| Diagnostic: `punch` alone | 581,140 | −156 | 89,112 | unchanged | Predictable small data saving; not separately additive |
| 2. `-fdata-sections` | 581,288 | −8 | 89,128 | unchanged | Reject as an optimization project: insignificant gain and broad code-layout change |
| `-fdata-sections` + `--gc-sections` | 581,288 | −8 | 89,128 | unchanged | Exactly the preceding image; GC contributes zero |
| `--gc-sections` | 581,296 | 0 | 89,128 | unchanged | Byte-identical baseline |
| `--relax` | 581,296 | 0 | 89,128 | unchanged | Byte-identical baseline |
| `--sort-section=alignment` | 581,296 | 0 | 89,128 | unchanged | Changes image bytes without reducing size; reject |
| `-Map ... --cref` | 581,296 | 0 | 89,128 | unchanged | Byte-identical baseline; useful diagnostics |
| `-s` | same load-section bytes | 0 | unchanged | unchanged | Strips metadata, not firmware bytes; breaks existing symbol-dependent extraction/check path |

All successful non-strip variants pass `build.check` and existing target budgets. Baseline main capacity is 98,304 bytes; low capacity is 29,496; pool capacity is 344,064 with the required 8,192-byte reserve. No capacity/partition change is included.

## Candidate 1: initialized state versus BSS

Source-backed patch: **`build/headroom-link-51/punch-bss.patch`**, modifying only private copies of `firmware/src/punch.c` and `firmware/src/main.c`.

`punch.c` initializes a 188-byte runtime state with only three nonzero bytes: `req=-1`, `cur=-1`, `fg=8192`. The independent 36-byte `punch_filter` has only `cur=-1` nonzero. Both are runtime state, not serialized project fields or factory content. Their initializers force all 224 bytes into `.data`, including their zero state.

The candidate removes those two declaration initializers. `fm1_cstart()` assigns the four original nonzero fields immediately after the existing `.data` copy, after BSS zeroing and before RAM-code copies, boot restoration, recovery and `fm1_main()`. All other fields are already zeroed by the unchanged startup loop. Entry assembly executes `cli`; `fm1_irq_init()` disables all source banks; audio setup/enabling occurs later. Thus neither object is exposed to playback with incomplete initialization. The recovery route also receives the original state. No extra per-block initialization, conditional branch or audio decoder is introduced.

| Combined candidate section | Baseline bytes | Candidate bytes | Delta |
| --- | ---: | ---: | ---: |
| `.text` (code and constants) | 577,756 | 576,492 | −1,264 |
| `.data` load payload | 524 | 300 | −224 |
| `.bss` | 88,600 | 88,808 | +208 |
| Main region extent | 89,128 | 89,112 | −16 |
| `.ram_text` | 2,956 | 2,956 | 0 |
| `.c1_text` | 60 | 60 | 0 |
| `.lowram` | 28,280 | 28,280 | 0 |
| `.pool` | 332,876 | 332,876 | 0 |

The **1,488-byte** saving is the actual total image delta, not 224 plus an estimated code saving. JieLi's global-merging pass changes which runtime variables share bases and short offsets when these objects move. `function-size-deltas.json` records the consequential changes: for example, `events_block` shrinks 120 bytes, `mix_block` 92, and `mix_part_run` 62. Their source algorithms are unchanged. Main RAM falls 16 bytes because the merged layout/alignment changes; moving data to BSS does not inherently free those objects' RAM.

The two split experiments demonstrate interaction: 156 + 1,220 = 1,376, whereas the combined measured result is 1,488. Future source edits may alter this compiler-layout bonus. Rebuild the final integrated source; do not promise the same saving forever, or sum it with other source-layout candidates.

### Validation and limits

- Full candidate target recompile/link repeated with identical image SHA-256: `16f9bdafba978b86188447b5da30a21952333f37daa3c50faa708d435fcb7102` (`punch-repeat.log`).
- Target placement/checks pass: 925 `.ram_text` instructions, no calls; core-1 loop remains 60 bytes; scratch, ROM-reference and region checks pass.
- All 24 monitored target render/ISR **loop instruction counts, divides, calls and weighted costs exactly match baseline**. Only `slicer_track`'s total function count decreases from 337 to 336, outside its measured loop. No monitored loop acquired work. See both `budget.log` files and `budgets.py`.
- Both native punch suites pass all 27 effects, boundedness, dry-after-release, quick switches and FX-held keyboard checks. Their initial 224 object bytes match exactly. Their complete **4,850,604-byte WAV files are byte-identical**, SHA-256 `2bc5f42f38af26c74b09363675e6c9d0c2e69f213d81d884510c7085dc45ec01`.
- Both broader native suites pass **182 unchanged goldens**, zero health failures, zero voice/routing failures, zero CPU entries over budget and zero crashes. These use the ordinary host preset/table path; they do not re-prove the separately integrated target boot-preset decoder. CPU timings are host diagnostics, not hardware cycle measurements.
- Native compilation uses the existing Zig 0.13 compiler, with private local/global caches. The candidate host tests explicitly apply the same four assignments at entry because host tests do not execute `fm1_cstart()`. Test adaptations are preserved in private trees and scripts.

**Integration obligation:** production hostsim/UI/test entry points currently rely on declaration initialization. Applying only the firmware patch while leaving those entry points untouched changes their initial state. Introduce a small shared cold initialization helper and call it from actual target startup and every relevant host bootstrap, or retain equivalent host declaration initialization behind an existing well-defined build configuration. Preserve the target measurement after that integration. This audit's patch is a measured prototype, not permission to leave test startup inconsistent. Do not lazily initialize from `punch_block` or add playback work.

The exact startup values, golden audio and static loop budgets are established. Physical boot/recovery, cache effects, stack high-water and hardware worst-case deadlines were not measured. No device was flashed.

## Retained sections and genuinely dead code

`firmware/app.ld` keeps `.text.crt0`, `.ram_text*` and `.c1_text*`; normal `.text*` and `.rodata*` are collected into XIP `.text`. The application link lacks `--gc-sections` even though the loader uses it. This initially looked promising, but actual GC discards only zero-sized input `.text` sections. The resulting application is **byte-identical**. Adding `-fdata-sections` still leaves GC with zero incremental saving. No useful dead retained section was demonstrated.

The single translation unit and static definitions let the compiler remove many unreachable routines before linking. The object relocation inventory finds only four nonempty C function sections lacking incoming references *within that C object*: `fm1_fault_c`, `fm1_alnk0_irq`, `fm1_timer5_irq`, `fm1_cstart`. All four have incoming assembly references, recorded by the link map/cross-reference table: fatal common, audio wrapper, timer wrapper and entry stub respectively. They are required, not dead.

The fatal-vector table is **768 bytes** of assembly stubs: 128 × 6. `fm1_irq_init()` computes each vector as `fm1_fatal_stubs + 6*n`, and the common handler recovers the vector from the saved return address. Only one symbol appears for all stubs. Counting unnamed stub instructions as unreachable or removing unassigned vectors loses the exception contract. No vector reduction is recommended.

Thirty-two C functions receive references from non-code object sections, principally engine descriptors' note-on/render/block/done/description callbacks. `inventory.json` lists every such function and relocation source. Direct-call searches would incorrectly label these unused. The active engine menu, preset application and voice rendering consume these descriptors. Generic callback dispatch already exists at engine/block boundaries; no new per-sample dispatch is proposed.

The packed preset/table restoration and the two-icon cleanup are already present. No saving from those prior integrations, disabled console/SLICE source, existing drum/limiter/texture BSS changes, `-Oz`, hot `DSP_INLINE`, LTO or toolchain replacement is counted here.

## Padding, merged globals and image accounting

The baseline load image is exactly `.text` 577,756 + `.ram_text` 2,956 + `.c1_text` 60 + `.data` 524 = **581,296 bytes**. `.lowram`, `.bss`, `.pool` and `.noinit` are NOLOAD; they consume RAM, not image payload. In particular, the relocatable object's large `.pool` may be PROGBITS, but the output linker section is NOLOAD. Object file byte size is not firmware flash use.

The link map explicitly records just **12 bytes of fill** in loaded sections: 2 before `.text.cpu1`, 8 before C `.rodata`, 2 at the end of `.ram_text`. They are not an invitation to violate assembly/vector, object or startup word alignment. A separate 4-byte address gap between `.data` end and `.bss` start consumes RAM, not image bytes. Internal struct/table padding remains possible, but is not represented by these link-map fills and must be assessed object by object. Blanket packing or section SUBALIGN is rejected.

Object C `.text*` inputs total **231,928 bytes**; object `.rodata*` inputs total **345,737 bytes**. These input totals must not simply be added to the output with assembly: `.rodata.str1.1` relaxes/merges from 10,017 to 9,172 bytes, **845 bytes already recovered by the linker**. Assembly text adds 926 bytes, and loaded `.text` fills add 10, reconciling the 577,756-byte output. String/suffix merging is already active; no further 845-byte saving is available.

`.L_MergedGlobals*` symbols overlap their individual member aliases. Their sizes cannot be summed with those aliases or read as unnamed dead allocations. The remaining `.data` inventory includes palette values, sentinel values, RNG seeds and compact UI state. Apart from the focused punch objects, the next sparse objects are `live_mod` (64 bytes, 9 nonzero bytes) and `dust` (32 bytes, 4 nonzero bytes). Those are follow-up measurement candidates only, not claimed savings; their boot assignments and new merged-layout effects could offset or interact with the benefit. There is no remaining multi-KiB initialized-zero blob in the baseline `.data`, whose entire payload is only 524 bytes.

## Emitted helpers, inlining and supported options

A comparison of complete function-section bytes **and relocation identities** finds just two exact duplicate groups in the target C object:

| Functions | Bytes each | Assessment |
| --- | ---: | --- |
| `dstep_mask` / `ota_rd16` | 10 | Both read a little-endian 16-bit value, but one is used by the playback sequencer and the other by cold OTA parsing. At most one 10-byte body before any call/layout effects; no new audio call justified. |
| `fm1_delay_us` / `fm1__wait` | 22 | Current target loops match, but input wait preserves the `FM1_INPUT_IDLE()` hook. Sharing must preserve test/alternate-build hook behavior. Tiny maintenance candidate, no measured saving claimed. |

The 32-byte gross duplicate-body bound is not a net image promise and does not cover semantic similarities, differently emitted specializations or inlined copies. It does rule out a large obvious pool of bit-identical emitted helpers under this test. Different engine functions and specialized DSP helpers are not safe to merge merely because they implement related math.

Existing tiny outlined helpers are often deliberate `UI_SMALL`/cold boundaries or callback targets. Existing hot helpers remain explicitly inlined. This audit did not add blanket `noinline`, forced common DSP helpers, audio-path generic dispatch, packing, fast-math or a replacement toolchain. Broad inlining/LTO experiments are already documented in `COMPILER-FLAGS-AUDIT.md`; they were read, not recounted as new gains.

The installed linker advertises GC, relaxation, section alignment sorting, maps/cross-references and stripping; all were exercised. It does not advertise ICF/function folding. `--relax` does nothing beyond the default result here. Sorting changes bytes but saves zero. Stripping leaves all four load-section payloads byte-identical (`strip/section-equivalence.json`), while removing symbols needed by the existing `symv()` extraction/check logic, so the normal build fails before assembling its final image. Debug/symbol reduction is not flash headroom. `-fdata-sections` changes compiler global layout and saves just eight bytes even without GC; it is not evidence of dead data removal.

## Integration recommendation

Take the **two-object punch startup candidate** as the only worthwhile measured new result from this audit. Make its host/target initialization contract explicit, repeat the final integrated same-version target and behavior checks, and retain a hardware boot/deadline gate. Budget **1,488 bytes only for this frozen combination**; rebuild when combining other specialists' work. Keep map/cross-reference diagnostics available for future audits, but do not adopt linker/ABI flags for speculative savings. No large dead retained section, padding windfall or duplicate-helper pool was found.

