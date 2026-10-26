# Ambient headroom: link, layout and size accounting

Audit: 2026-10-24, MerthSloop **2.8.0.58**, current working sources rather than HEAD. Production sources, shared generated assets, shared build outputs, installer, version and Git state were not modified by this audit. All experiments are in `build/headroom-ambient/link/`; this report is the only deliverable outside that scratch directory.

**Finding:** no multi-KiB linker, alignment, garbage-collection or exact-duplicate-code win remains demonstrated. A new private combined cold-initialization experiment saves **288 linked application bytes**, with unchanged main/low/pool RAM. It raises flash margin from **948 to 1,236 bytes**. Treat it as an optional integration candidate, not adequate architectural room for an unspecified ambient/drone engine. The existing compiler and layout should stay in place.

## Frozen inputs and reproduction

`freeze.py` copied the actual dirty `firmware/`, `tools/`, `tests/` and existing `build/gen/`: **321 files**, including current generated sample/table assets. It did not export HEAD or regenerate shared assets. `manifest.json` hashes every copied input; manifest SHA-256 is `2b00ec11c3629277db6628cd52e66183e02bffa7b8b654e4b9215eedcdb9ee12`. Shared reference ELF/object/disassembly/application were copied before experiments. `final-verification.json` reports **zero shared input changes since freeze**; private changes are exactly `fx.c`, `voice.c`, `main.c`. Parent commits made later do not change this frozen comparison.

Installed toolchain: `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, Clang **4.0.1**, via WSL Ubuntu. `toolchain.txt` and `toolchain-sha256.json` retain executable identity. No install or global setting changed. Baseline C flags are `-target pi32v2 -Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, with `FELUCCA_BOOT_TABLES=1`, the ordinary two-pass `FELUCCA_BOOT_PRESETS` probe/pack/final compile, `FELUCCA_SLICE=0`, `FM-1_900`, unchanged source version and default remaining feature switches. Assembly, linker script and region sizes are unchanged. Full per-build arguments are in `commands.json`.

The private driver imports its private `tools/build.py`, calls only `build_app()` and `check()`, and writes under private `source/build/`. Preset packing runs there; no shared generator, normal packaging entry point, loader build, installer or device is invoked. Baseline image is **byte-identical to the copied shared .58 application**:

`f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`.

Reproduction from the repository in PowerShell, using the already frozen tree:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 prepare.py baseline
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 run.py baseline
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 run.py data-gc
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 gc_only.py
# For each of dust, live, both: prepare.py NAME, then run.py NAME.
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 prepare.py both
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 run.py both
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 repeat.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/link --exec python3 validate.py
python build/headroom-ambient/link/oracle_target.py
python build/headroom-ambient/link/finalize.py
```

Do not rerun `freeze.py` to reproduce the frozen results: it copies the then-current shared source. `prepare.py` restores its saved original three files before applying each experiment. Build variants sequentially because they share the private intermediate directory. Saved result directories are authoritative. Analysis scripts `inventory.py`, `accounting.py`, `deltas.py` read saved ELF/object/map files. `finalize.py` writes the reviewable patch and input verification. `accounting.json` was captured before the final candidate and therefore describes that earlier private-input state; `final-verification.json` is the final source-integrity record.

## Linked measurements and ranked disposition

These are complete target links with the real region limits; no enlarged XIP region, host-size extrapolation or sum of independent gains is used.

| Candidate | Application B | Delta vs baseline B | Main static RAM B | Low RAM / pool B | Disposition |
|---|---:|---:|---:|---:|---|
| Baseline .58 | 583,388 | 0 | 86,176 | 28,928 / 332,876 | Exact shared-image reproduction |
| **1. `dust` + `live_mod` cold initialization** | **583,100** | **−288** | **86,176** | unchanged | Optional small candidate; startup integration gates below |
| `dust` alone | 583,356 | −32 | 86,176 | unchanged | Too small alone |
| `live_mod` alone | 583,436 | +48 | 86,192 | unchanged | Reject alone: larger image and RAM |
| `-fdata-sections` + `--gc-sections` | 583,384 | −4 | 86,176 | unchanged | Reject: trivial layout change |
| `--gc-sections`, saved baseline objects | 583,388 | 0 | unchanged | unchanged | Every loaded section byte-identical |

All five full-build variants pass `build.check`, target budget checks and target inlining checks. The GC-only experiment separately relinks the original saved objects; `gc-only/equivalence.json` compares all four loaded-section payloads exactly. It discards only zero-size `.text` inputs. The four-byte combined data-section/GC change is not evidence of removable dead data; GC alone proves no retained byte can be recovered that way from the baseline.

### Optional 288-byte candidate

Patch: `build/headroom-ambient/link/cold-state.patch`. `dust` is 32 bytes with one nonzero 32-bit seed, `0x2545F491`; `live_mod` is 64 bytes with only nine nonzero field bytes. Both are runtime state, independent of serialized project formats. The patch removes declaration initializers and restores the original values with ten scalar assignments just after `punch_boot_init()` in `fm1_cstart()`, after BSS zeroing and `.data` copying, before recovery or normal startup. No playback function, decoder, event algorithm, audio buffer or preset content changes.

| Combined candidate section | Baseline B | After B | Delta B |
|---|---:|---:|---:|
| `.text` (code + constants) | 580,072 | 579,880 | −192 |
| `.data` flash load image | 300 | 204 | −96 |
| `.bss` | 85,872 | 85,968 | +96 |
| `.ram_text` / `.c1_text` | 2,956 / 60 | unchanged | 0 |
| `.lowram` / `.pool` | 28,928 / 332,876 | unchanged | 0 |
| Main occupied extent, including section gap | 86,176 | 86,176 | 0 |

The extra 192-byte code reduction is a **compiler global-layout interaction**, not eliminated signal processing. The compiler's merged globals and addressing change when state moves. For example `mix_block` shrinks 140 bytes, `uac_render_start` 64 and `layer_screen_draw` 54, while `mix_part_run` grows 70 and startup grows 92. `function-deltas.json` records the complete differences. Moving 96 bytes from DATA to BSS intrinsically saves neither 96 RAM bytes nor 288 flash bytes on every future source version. The separate experiments even predict a net increase if incorrectly added together. Rebuild any combined integration.

An independent full rebuild reproduces **583,100 bytes** and SHA-256 `54ca6c23780fcfb7a97665347b4ec76e6e1c3eb159ed04014b81eec746116b88`; see `both-repeat.json`.

Validation:

- `validate.py` compiles the exact original declarations and proposed assignments using existing Zig 0.13, with both caches private to this scope. All **96 initialized state bytes match**, including padding; `oracle_target.py` also matches them against the actual baseline target ELF bytes. This is an initialization oracle, not a complete firmware execution test.
- Every monitored loop's instruction/divide/call/weighted-cost metrics match baseline for every variant. Existing `target_budget.py` and `target_inlining_test.py` pass without budget edits. RAM flash-driver code stays 925 instructions with no calls; core-1 code stays 60 bytes.
- No full audio-golden run, actual UI interaction suite or hardware boot/recovery/deadline measurement was performed for this prototype. Equal startup bytes and unchanged loop metrics support further integration; they do not replace those checks or prove hardware speed.

**Integration gate:** preserve every host bootstrap's existing declaration-initialization contract. The prototype only establishes target startup and the isolated state oracle. Before adoption, introduce explicit once-per-cold-start initialization in every applicable native/WASM/preview/test bootstrap, or preserve original declarations for builds that do not execute target startup. Do not call initialization per note or per block, reset live modulation during project changes, or restart the dust RNG during playback. Run real boot-equivalent audio goldens, modulation/punch/UI regressions, target checks, and physical cold/recovery boot. Adding a shared helper can change the measured 288-byte saving; report the final integrated result.

Upstream burden is low for the three-file prototype, moderate for correctly maintaining multiple host entry points. No project/wire/schema migration is needed. This is a minor reserve, not a reason to expand scope into scattered one-byte sentinel conversions: only **204 bytes total `.data`** remain after it, much of which genuinely contains nonzero defaults.

## What currently occupies flash

The image reconciles exactly:

`580072 .text + 2956 .ram_text + 60 .c1_text + 300 .data = 583388 bytes`.

The target C object has **234,080 `.text*` bytes** and **345,910 `.rodata*` bytes**. Assembly adds 926 code bytes. String merging already reduces `.rodata.str1.1` from **10,046 to 9,197 bytes** (849 recovered); five bytes of XIP-section fill complete the 580,072-byte `.text` accounting. Another two fill bytes are inside `.ram_text`, for seven across all loaded sections. These input categories must not be confused with `.text` output, which includes both code and constants. Summed named function symbols are 237,094 bytes, also including RAM code; named XIP object symbols are 335,807 bytes. These symbol totals are inventories, not a disjoint reconstruction of all output or a savings estimate.

Largest retained functions and data, from `accounting.json`:

| Function | Target B | Constant | Target B |
|---|---:|---|---:|
| `mix_block` | 16,106 | `SMP_DATA` | 303,054 |
| `events_block` | 11,028 | `BOOT_DS_BASE` | 2,831 |
| `layer_screen_draw` | 5,986 | `BP_DATA` | 2,813 |
| `ed_handle` | 4,428 | `BOOT_FONT_S_DATA` | 1,968 |
| `mix_part_run` | 3,466 | `BP_NAMES` | 1,542 |
| `drums_mix` | 2,986 | `SMP_ZONES` | 1,428 |
| `usb_poll` | 2,448 | `DS_KITS` | 1,408 |
| `ui_input` | 2,228 | `TP` | 1,136 |

Factory sample bytes alone occupy **51.95%** of the image. The largest three functions total 33,120 bytes, but much of `mix_block`/`events_block` is inlined live processing/control. Their large symbol sizes do not establish dead features or permission to outline their sample loops. UI/DSP cleanup and alternate asset representations belong to the other audits; this report provides current sizing rather than duplicating their proposals.

### Retention, duplicates and padding

The only nonempty C function sections without incoming references from another section in that same C object are `fm1_fault_c` (188 B), `fm1_alnk0_irq` (734), `fm1_timer5_irq` (894) and `fm1_cstart` (1,440). Assembly references all four; they are required exception/ISR/startup entry points. There are **32 function sections referenced from non-code sections**, principally engine descriptor callbacks. Direct-call-only dead-code searches would misclassify these.

Exact emitted-body comparison, including relocation identities, still finds only `dstep_mask` / `ota_rd16` (10 B each) and `fm1_delay_us` / `fm1__wait` (22 B each). Thus the obvious duplicate-body bound is **32 gross bytes**, before call/address/layout effects. The first spans sequencer and cold OTA paths; the second preserves an alternate-build idle hook. No new audio call or upstream semantic coupling is warranted for that bound. Similar-but-different functions require source-level proof by their owning audit.

Loaded-section map fills total **seven bytes**: two before CPU-1 assembly, one before `.rodata.cst4`, two before `.rodata.cst32`, two at the end of `.ram_text`. The four-byte `.data`→`.bss` address gap occupies main RAM, not flash. `.L_MergedGlobals` symbols overlap their member aliases and are not additional anonymous allocations. Blanket packing, section SUBALIGN or reordering cannot credibly deliver KiB from these gaps and can break word-copy/ABI requirements.

## RAM and structural limits

| Region | Current occupied / capacity B | Meaning of remaining space |
|---|---:|---|
| Application load image | 583,388 / 584,336 | **948 B** actual flash margin; 1,236 with private candidate |
| Main static RAM extent | 86,176 / 98,304 | **12,128 B** static margin; does not prove stack high-water or a safe shared lifetime |
| Low RAM below `0x01c07f00` | RAM code 3,016 + low data 28,928 / 32,512 | **568 B** real shared-region margin, not 29,496 bytes newly available |
| Pool | 332,876 / 344,064 | 11,188 B address gap, of which **8,192 B reserve must remain**; only **2,996 B** above that floor |
| NOINIT | 15,464 / 15,696 | 232 B inside a persistent boot/debug region; not interchangeable general zeroed RAM |

Moving persistent arrays between main RAM and low RAM can rebalance RAM but does not reduce their flash load unless representation/initialization also changes. Moving raw executable code to RAM likewise keeps its bytes in the flash load image and consumes RAM. The existing RAM flash routines and CPU-1 loop have execution-while-flash-busy obligations; placing arbitrary cold code there adds pressure to the already 568-byte low-RAM margin. Moving buffers into the pool consumes its reserve, not free capacity.

Main RAM can in principle fund another cold-restored metadata representation or a proved executable overlay. It is **not a measured flash saving**. Even cold startup symbols `fm1_cstart` + `fm1_main` total only 3,188 bytes, and `fm1_main` contains the permanent event loop; they cannot both simply be discarded after boot. An overlay/compressed-code design needs target relocation/far-call handling, interrupt/core-1 reachability, instruction-cache/guard treatment, explicit lifetimes and a linked decoder/veneer measurement. No such design is validated here. Avoid buying uncertain KiB with an interpreter or per-sample decoder. Larger sound-preserving wins should come from measured source sharing or approved cold data changes, not generic layout switches.

The existing 584,336-byte app capacity already includes the previously recovered **2,772 bytes** within the encrypted `0x4000..0x93000` boundary. That gain is integrated and cannot be claimed again. The apparent `0x93000..0x97000` flash gap still intersects SDK VM/update-record/SFC contracts documented in `ARCHITECTURAL-HEADROOM.md`; it is not another allocatable 16 KiB. Repartitioning remains a separate hardware/recovery project with high upstream and migration burden.

## Rejected work and handoff

Read before experiments: `HEADROOM-LINK-AUDIT.md`, `COMPILER-FLAGS-AUDIT.md`, `ARCHITECTURAL-HEADROOM.md`, `OPTIMIZATION.md`, `HEADROOM-ASSETS-AUDIT.md`, `HEADROOM-STORAGE-AUDIT.md`, `HEADROOM-UI-AUDIT.md`, and `REDUNDANCY-AUDIT.md`. Integrated punch BSS initialization, boot tables/presets, ragged trims/font metrics, obsolete UI rendering removal, direct legacy conversion and the aligned application slot are baseline content, not new recommendations.

Keep `-Oz`, current hot `DSP_INLINE` boundaries and the pinned compiler. Prior `-Os`, blanket noinline, jump-table removal, internal inlining thresholds, merge-function flags and LTO trials already have measured limitations. Installed-wrapper LTO previously saved only 32 bytes; plain-ld LTO and merge-functions were unsupported. None was repeated as a supposed new avenue without new evidence. ICF is not demonstrated by this linker. Stripping metadata does not reduce loaded firmware bytes and breaks symbol-dependent build extraction. Relax/sort trials already supplied no useful gain. No global toolchain/flag change is recommended.

1. **Optional bounded follow-up:** integrate the supplied combined cold-state patch only with a complete host startup contract and the checks listed above. Maximum established benefit is **288 B for this frozen source**, not an additive promise. Preserve comments explaining original modulation defaults during any polished integration.
2. **For multi-KiB headroom:** use this current inventory to prioritize the separate UI/DSP/static-data specialists' measured source candidates, then build their actual combination. No link-only mechanism here supplies that budget.
3. **Do not consume RAM gaps casually:** keep the 568-byte low-RAM margin and 8,192-byte pool reserve explicit when budgeting ambient/drone state. Main RAM has more room, but moving state there must preserve ownership/target timing.
4. **Retain reproducible diagnostics:** maps/cross-references can remain an opt-in audit mode with zero demonstrated payload cost. The saved ELF/disassembly, relocation-aware duplicate scan and exact section reconciliation are the concrete handoff for future work.

No candidate was integrated, packaged, flashed, published, committed or pushed by this audit. Hardware timing, power-loss behavior and cold/recovery boot remain unmeasured; host and static target checks are not hardware performance evidence.

## Subsequent integration remeasurement

The combined cold-state initializer was remeasured while fitting the About restoration and EVOLVE octave bounds. It enlarged that newer target and was not adopted. The original declaration defaults and cold-boot calls remain in production. The frozen 288-byte result above is therefore not an additional available saving on the current source.
