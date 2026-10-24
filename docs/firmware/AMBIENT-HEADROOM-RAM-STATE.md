# Ambient headroom: low RAM and ordinary static state

Audit: 2026-10-24, MerthSloop **2.8.0.58**, frozen from the dirty working files. Production sources, shared generated/build outputs, installer, version, Git refs and packages were not changed by this audit. Experiments and reviewable patches are isolated under `build/headroom-ambient/ram-state/`.

**Recommendation:** the measured combination of insert-state union, sample-upload scratch sharing and FFT scratch sharing recovers **880 B main static RAM + 1,024 B pool**, and saves **48 B flash**. It fits the existing slot. Separately, the entire **6,156 B factory preset bank can move out of low RAM**, consuming **6,160 B main RAM** including alignment. That is relocation, with **4 B more total RAM**, not a saving. Keep the relocation optional until the ambient/drone and boot-packing resident-memory budget is settled.

The FFT recovery duplicates the outcome in [RAM buffers and lifetimes](AMBIENT-HEADROOM-RAM-BUFFERS.md). The two implementations are alternatives: this audit uses the existing second main-scratch FFT vector; that audit destructively reuses the consumed waveform. **Do not count 1,024 B twice or apply both aliases.** All combined measurements below use this audit's alias.

## Frozen baseline and exact target comparisons

The private target baseline reproduces the requested **583,388 B** application, SHA-256 `f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`. `manifest.json` records **175 firmware/tool/generated inputs** copied from actual working files, not exported from HEAD. Tests were copied separately into the private tree. `input-verification.json` confirms the restored private source matches all 175 inputs. The only later shared-input difference at verification was the parent's `tools/web_provenance.py`; all frozen firmware/generated inputs still match. Parent commits do not redefine this baseline.

Toolchain: installed JieLi `20250324.1`, Clang **4.0.1**, `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, running through WSL Ubuntu. Flags: `-target pi32v2 -Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, normal HAL/src/gen includes, `FELUCCA_BOOT_TABLES=1`, two-pass raw-object preset probe/packer and `FELUCCA_BOOT_PRESETS=1`, `FELUCCA_SLICE=0`, `FM-1_900`, unchanged .58 source version and remaining feature defaults. Linker script and real **584,336 B** application slot are unchanged. `run.py` imports the private build module and calls only `build_app()` and `check()`. No SDK packaging, loader, shared generator or installer build runs. Full arguments, ELF, object, probe, sections, symbols, disassembly, image and checks are retained per result directory; `measurements.json` collects their identities.

These are complete target links, not host-size or symbol-only savings. Independent rows are alternatives; only the explicitly named combinations were linked together.

| Result directory / candidate | Application B | Flash delta B | Main RAM B | Low RAM B | Pool B |
|---|---:|---:|---:|---:|---:|
| `baseline` | 583,388 | 0 | 86,176 | 28,928 | 332,876 |
| `insert-union` | 583,340 | -48 | 85,808 | 28,928 | 332,876 |
| `sample-scratch` | 583,388 | 0 | 85,664 | 28,928 | 332,876 |
| `fft-scratch` | 583,388 | 0 | 86,176 | 28,928 | 331,852 |
| **`combined`: all three reductions** | **583,340** | **-48** | **85,296** | **28,928** | **331,852** |
| `presets-main`: relocation alone | 583,388 | 0 | 92,336 | 22,772 | 332,876 |
| **`combined-presets`: reductions + relocation** | **583,340** | **-48** | **91,456** | **22,772** | **331,852** |

| Margin | Baseline | Combined reductions | Combined + preset relocation |
|---|---:|---:|---:|
| Flash free | 948 | 996 | 996 |
| Main static free, capacity 98,304 | 12,128 | 13,008 | 6,848 |
| Low RAM free, capacity 29,496 | 568 | 568 | 6,724 |
| Pool gap, capacity 344,064 | 11,188 | 12,212 | 12,212 |
| Pool gap **after retaining 8,192 reserve** | 2,996 | 4,020 | 4,020 |

Main RAM is the linker extent from `_data_start` through `_bss_end`, including alignment. Baseline `.data` is 300 B, `.bss` 85,872 B, with four bytes between them: 86,176 B total. Combined `.bss` is 84,992 B; `.data` and the four-byte gap remain unchanged. Low RAM is `0x01C00BC8..0x01C07CC8` in the baseline; its permitted end is `0x01C07F00`. The pool/noinit/stack boundaries do not move.

## Ranked implementation handoff

### 1. Share the insert's mutually exclusive private state

Patch: `insert-union.patch`, one private `fx.c` change. Keep the common wet gain, running type, counter/write position, phase and DC blocker outside an anonymous union. Union the drive/crush private filters/held sample, phaser all-pass/feedback state, and flanger/chorus delay line. The three members occupy 16, 36 and 512 B; the maximum is 512 B.

The ordinary `ins_t` falls **580 -> 528 B**, and seven channels fall **4,060 -> 3,696 B**, a **364 B payload reduction**. The complete link recovers **368 B main RAM** due to an additional four-byte layout improvement. Total image saves **48 B**, including every code/alignment effect; these are not separate additive estimates.

Correctness rests on `ins_run`, not merely on UI type exclusivity. It continues rendering the *old* type while wet gain fades to zero. Only at zero may the running type change, and a nonzero restart clears the entire object with the existing `memset` before processing the new type. No crossfade processes two private members simultaneously. `ins_rest` also clears the complete object. The union does not change this state machine, add a test/helper/copy to a sample loop, or move state between cores. Per-part state remains owned by its assigned render unit; four drum channels remain distinct. Alignments stay natural (four-byte union alignment), with no packed or unaligned DSP fields.

Target evidence: `ins_drive` remains **273 instructions / 196 loop instructions / zero loop calls**. `ins_swept` falls **338 -> 327 instructions**, **214 -> 207 loop instructions**, with zero loop calls/divides in both. `mix_block` and `mix_part_run` have identical instruction, loop-call and divide counts. These are static disassembly observations, **not target cycle or deadline measurements**.

Upstream burden: **low**, confined to an internal runtime type in the upstream insert implementation; no project, preset, wire or callback ABI changes. Keep the type-change/clear contract explicit in a lifetime comment, update the old 580-byte explanatory comment, and remeasure after upstream inserts gain new modes. The private patch retains the original comment so its literal measured source can be reviewed; the stale comment is not a new allocation requirement.

### 2. Put sample-upload/backup decode staging in the existing main-loop union

Patch: `sample-scratch.patch`, private `editor.c` and `scratch.h` changes. Add a 512-byte `smp` member to the already 3,840-byte `main_scratch` union, include that header at the use boundary, and replace the separate aligned `ed_smp_buf` allocation with the member. A compile-time bound requires room for the complete 480-byte user-sample header. Union alignment remains four bytes. **512 B main RAM recovered; no flash, low RAM or pool change.**

All reads end inside one editor request. WRITE unpacks at most 256 bytes, optionally erases a sector, writes those same RAM bytes through `fl_write`, and then constructs its reply. END unpacks/validates the header, checks source data CRC, writes the header, and calls `smp_user_scan`, which has its own validation buffer. Backup data chunks are copied into `proj_tmp` immediately; commit happens in a later request after the decode buffer's last read. The direct flash erase/program functions do not call the storage payload transaction or a UI draw. IRQ/audio/core 1 do not own this member. `ota_idle`/wire sending occurs after sample staging's last read. No pointer into staging survives a request.

Do **not** extend this alias to `ed_out`, `sx_frame`, `proj_tmp`, `usr_zone` or `smp_user_scan.zs`. A reply header may already exist when a storage helper uses `main_scratch.st`; received SysEx stays live while replies are assembled; staged banks/projects are live through storage calls. Live zones must survive validation and IRQ-off publication. The proposed member is only the short-lived upload/decode buffer.

Upstream burden: **low to medium**, two localized files and an additional documented scratch lifetime. Future asynchronous flash writes or callbacks that draw/load storage inside a request would require rechecking the proof. The patch does not make such callbacks safe by adding a lock or a persistent copy.

### 3. Reuse the unused second FFT vector in main-loop scratch

Patch: `fft-scratch.patch`, one private `ui_vis.c` change. Bind real and imaginary vectors to `main_scratch.vis[0]` and `[1]`; the existing member is already `int16_t[2][512]`. Remove the separate 1,024-byte pool `vis_im`. The real vector occupies exactly the same first 1,024 bytes as the old `main_scratch.snap` alias. The second vector is disjoint from it and from persistent `vis_frame` waveform/trail history. The largest union member remains the 3,840-byte storage payload: **zero destination growth, 1,024 B true pool reduction, unchanged flash/main/low totals**.

`vis_fft` initializes every imaginary sample before using it. FFT and immediate band extraction finish within `vis_update`, before canvas rendering; no storage/OTA/editor callback occurs inside that lifetime. Interrupts continue using their separate audio scope producer. HOME snapshots, storage and OTA may overwrite scratch between frames because neither FFT vector is persistent state. No new audio work or copying is introduced.

Upstream burden: **low**. Update the existing scratch comment from its historical left/right description to real/imaginary and retain the size assertion. The [RAM-buffer audit](AMBIENT-HEADROOM-RAM-BUFFERS.md) supplies an independently tested alternative using `vis_frame.scope.l`; choose one. Its 1,024-byte result is the **same recovery**, not additional headroom. Swapping aliases in a larger combination requires a fresh combined link.

### 4. Move boot-restored presets when low resident memory is actually needed

Patch: `presets-main.patch`; `combined-presets.patch` is the complete reduction-plus-relocation alternative. Remove `.lowram` only from the boot-restored factory preset declarations in eleven active engine files, the optional SLICE file, and the SAMPLE preset declaration emitted by `tools/gen_samples.py`. The private experiment also edits its copied generated sample header to represent that generator change without regenerating shared assets. Raw const initializer tables and the boot packer/initializer API remain supported.

There are **171 x 36 B = 6,156 B** of presets across twelve active tables. These are ordinary `preset_t` objects with 32-bit name pointers and 32 payload/pad bytes, accessed through normal engine pointers. They have **no low-address, DMA, near-pointer, RAM-execution or assembly stack requirement**. Moving them to ordinary zeroed `.bss` changes destinations and relocations, not stored IDs, names, field values or indexing. The measured image size is unchanged. Main growth is **6,160 B**, including four bytes of alignment; low reduction is **6,156 B**. Total RAM therefore grows four bytes. This does not recover any capacity for the sample/engine pool by itself. Individual preset banks can be relocated independently using the object sizes in the appendix, but no linked subset result is claimed here; alignment effects must be measured. Freed low data capacity also does not automatically become an executable overlay: the current RAM-code/PC guard window ends at `_rt_end` after `.c1_text`, before `.lowram`. Executable placement would need its own startup, linker and guard proof.

Startup order stays: interrupts disabled; BSS/pool zeroed; low RAM tested/zeroed; data copied; explicit punch initialization; RAM/core-1 code copied; numeric tables restored; factory presets expanded using `proj_tmp`; mailbox/guard setup; recovery or normal startup; persistent storage/audio/core-1 startup later. No code consumes preset pointers before their initializer completes. The destination remains persistent after boot. Zeroing BSS instead of low RAM supplies the same initial state, and the existing initializer fills every payload/name pointer.

Upstream burden: **medium**, a one-time placement policy across factory table wrappers plus the sample generator. Do not edit only generated output. Preserve the conditional SLICE declaration policy for an enabled build, and continue raw-table host/probe builds; the measured shipped configuration has SLICE disabled. This audit does not claim a successful full optional-SLICE target link.

## Complete region and major-state accounting

`inventory.json` preserves all sized symbols including compiler merged-global containers. `named-state.csv` removes those overlapping `.L_MergedGlobals*` aliases and classifies **639 actual named objects**. `major-state.md`, reproduced at the end of this report, classifies **all 113 named objects of at least 128 B**. Treat compiler merged-global labels and scalarized `ed_w.*` fields as layout containers/parts, not extra duplicated allocations.

| Baseline region / category | Exact accounting | Conclusion |
|---|---|---|
| Low RAM | 3,072 core-1 stacks + 17,368 reverb history + 6,156 presets + 1,280 per-core accumulators + 256 per-core part buffers + 512 drum streams + 284 per-core parameter snapshots = **28,928** | All low RAM is explained; no hidden multi-KiB dead reservation |
| Main BSS | 85,717 named payload + 155 alignment/unlabelled bytes = **85,872** | Do not sum merged-container aliases again |
| Main data | 287 named payload + 13 alignment bytes = **300** | Sparse punch initialization is already integrated; cold init of remaining small state belongs to the separate link audit |
| Main region extent | 300 data + 4 inter-section gap + 85,872 BSS = **86,176** | Static margin, not stack high-water |
| Restored main tables | **23,130 B** across numeric/audio/font/drum/FM6/FIR tables (see complete inventory) | Resident tables are intentional RAM-for-flash trades; font changes are not this audit's focus |
| Persistent main `.scratch` input | 3,816 whole-project undo + 2,400 visualizer frame/history = **6,216** | Ordinary BSS, not reusable transient flash-driver RAM |
| Pool | Eleven listed objects sum to **332,876** | Keep reserve; separate effects/track histories coexist |
| No-init | **15,464 / 15,696**, of which four project slots use 15,264 | Remaining 200 B are boot/debug records; 232 B gap cannot hold project/scratch buffers |

The low-RAM stacks are emitted as zero-size boundary labels around reserved storage, so they must be counted explicitly rather than omitted by a size-filtered symbol report. Neither the 568-byte low-RAM gap nor any preset relocation is measured core-1 stack space.

### Layout, widths, bounds and derived state

`type-probe.py`, `types.json`, `types-layout.log` and the field JSONs are **target-compiled unlinked ABI measurements**, separate from linked savings:

- `voice_t` is **84 B**, with 81 field bytes and three padding bytes. Its 32-bit phases, envelope/control arithmetic, sample positions, age and signed filter state are not interchangeable with 8/16-bit display values. Plain field reordering cannot make an 81-byte naturally four-aligned array element smaller than 84 B. Packing would change hot accesses and is rejected here.
- `track_t` is **2,272 B**, four instances **9,088 B**. Target offsets include voice array 152, pattern union 1,012, `p_old` 2,114. Its direct fields occupy 2,249 B plus 23 outer padding bytes; natural reordering could at best remove **20 B per track (80 total)** without changing field widths, before any linked code/alignment effect. The embedded voices contain an additional **96 B total padding** already included in the 9,088 B. No linked saving is claimed for reordering or packing.
- The drum track physically contains **672 B** of synth voice slots (`8 x 84`), while drums have their own live voice storage. This is a plausible structural reduction, **not a proven removable symbol**. `track_t *` APIs, contiguous `trk` arithmetic, generic reset/stop paths, host bootstrap and field addressing assume one uniform runtime type. Replacing embedded voices with pointers would introduce hot-path loads and initialization obligations; splitting the drum runtime needs a separate end-to-end refactor. It is deferred rather than advertised as a free 672 B saving.
- `preset_t` is **36 B**: 35 field bytes plus one pad. Removing 171 padding bytes by packing would unalign subsequent name pointers and violate the packer's explicit four-byte-pointer/32-byte-payload ABI unless the whole representation/access scheme changes. Retain the ABI. The separate boot-packing audit addresses stream representation without shrinking the live record.
- Synth/drum steps are **10 B**, naturally byte aligned; they already union inside each track. `P_COUNT=71`, `G_COUNT=59`, four tracks and eight voice slots are frozen target constants. The 284-byte low-RAM `pnew` is two `71 x int16_t` snapshots, not an oversized four-track array; rows and 1,280-byte accumulators may be live on both cores simultaneously.
- `p` and `p_old` are distinct through sounding-preset fades. Lock base/set/current values carry restoration semantics, not redundant copies. `fm6_patch` (468 B), `fm6_eff` (600 B), factory patches (1,024 B) and arena operator state serve mutable, derived, immutable and active-render lifetimes respectively; removing the cache would shift work into audio. No such cost is assumed free.
- MIDI ownership is already nibble packed (`midi_sel_on`, 1,024 B), with distinct press-channel/source/note ownership for release/revoice. Do not discard ownership because a note fits seven bits. `ft_ev` uses 16-bit block timestamps/durations: the 24-second bound is **33,075 blocks**, and `0xFFFF` means open, so a byte is insufficient. Strum events have 32-bit pending sample countdowns; no smaller bound has been established here.
- User-zone records are **28 B with no padding**, matching the saved header ABI. Offsets, sample counts up to 163,840 and Q16 rates up to 262,144 cannot all become 16-bit fields. The **1,792 B** live zones and **448 B** scan staging are intentionally separate until all records validate and an IRQ-off copy publishes them. `usr_nz=0` does not alone prove existing voices stopped reading old zones. The 480-byte complete header and 256-byte write chunk both fit the proposed 512-byte staging member.
- `proj_tmp` is **3,840 B**, larger than the current 3,816-byte project because legacy formats/bank/backup/OTA users share the union. No shrinking to only today's project size is recommended. The old 2,584-byte `proj_v3_tmp` removal is already integrated and is not counted again.

### Lifetime candidates deferred or rejected

| Candidate | Evidence / possible scope | Decision |
|---|---|---|
| Share 648-byte `layer_screen_draw.tl` with main scratch | Every draw clears/rebuilds all 27 typed tile records and consumes them synchronously | **Unmeasured next candidate**. Needs a shared typed declaration usable by standalone storage/host variants, pointer alignment and proof that LCD helpers do not overwrite the tile member. Avoid casting a raw byte array to a pointer-rich type merely to claim 648 B. No linked saving reported. |
| Share 448-byte `smp_user_scan.zs` with cold scratch | Boot/upload scan transaction, no persistent pointer needed after IRQ-off publish | **Unmeasured next candidate**. Preserve old live zones and optional SLICE scan order. Early engine inclusion precedes `scratch.h`'s boot-table declarations, so a naive include creates startup/type-order dependencies. No linked saving reported. |
| Overlay spectrogram and note trails | 3,456 B spectrogram versus 2,400 B frame/history union | **Reject unchanged-behavior claim**: spectrogram history survives switching away and back; trails clear on entry. Unifying storage changes return-to-style output. |
| Overlay project undo, `song_keep`, autosave or staging | Three 3,816-byte records plus 3,840-byte staging | **Reject**: undo survives arrangement playback, current/old projects coexist in import/exchange, and storage payload scratch is live while staged source/destination are needed. |
| Overlay UI scratch and audio buffers/canvas | Main may be interrupted during a draw/flash operation | **Reject**: UI exclusion is not ISR/core exclusion; LCD DMA also outlives drawing helpers. |
| Remove/reduce output, ASRC, MIDI or DMA rings | DMA buffer 4,096; USB ring 2,048; MIDI out 2,048; UART RX aligned 16 | **Reject without new hardware/queue evidence**: asynchronous consumers, USB rate conversion, note-off bursts and DMA ownership are real requirements. |
| Shrink delay/punch/slicer/engine arenas | Dominant pool allocations, already per-part/model unions | **Reject**: changes history lengths, polyphony, precision or sound. No runtime sample decompression, codec change or extra audio copy is proposed. |
| Move all 17,368-byte reverb history to main | Placement itself has no preset-like ABI benefit | **Does not fit** the baseline or 13,008-byte post-reduction main margin; splitting/rebalancing regions would only relocate budget and add complexity. |
| Reorder/pack all runtime structures | Track padding upper bound is small; descriptors already optimized | **Defer**: at most 80 B from track reordering before code effects; hot packed accesses and upstream conflict are unjustified. |
| Historical scratch/preset/table/icon wins | Earlier architectural/storage/link/asset/UI/compiler reports and OPTIMIZATION reviewed | **Already integrated or rejected**, not new gains. The RAM-buffer audit's audio scratch overlays also cost over 1 KiB flash and do not fit; this report does not revive them. |

## Stack, startup and region requirements

Core 0 user stack is `_ustack_lo=0x01C74100` to `_ustack_top=0x01C7A000`: **24,320 B**. Supervisor stack is `0x01C7A100..0x01C7C000`: **7,936 B**. Two separate **256 B guard bands** precede them. Core 1 reserves **2,816 B user + 256 B supervisor**, eight-byte aligned, inside `.lowram`. Code/stack addresses are used by startup assembly and core-1 launch; leave those contracts intact. The render's function and argument handoff and `csync`/join ownership are unchanged.

The existing console stack paint/high-water and low-RAM pattern test are instrumentation, not data collected during this audit. Main static free is not a measured reserve for any stack. No stack is shrunk, moved, shared with undo or inferred safe from host stack use. No physical device was available for nested ISR depth, core-1 maximum stack, bus/cache timing, LCD DMA or boot/watchdog latency. Low-RAM test success on hardware remains the platform's precondition; a successful link cannot establish it.

Presets and decoded numerical tables are live for the entire boot, even when the UI is elsewhere. Initialization must remain before recovery/normal operation and before exposing pointers to audio/core 1. Main scratch used by boot restoration becomes reusable after initialization; the expanded destinations do not. No candidate changes saved project/patch layout, optional engine IDs, pointer width, boot mailbox, top reserved memory, partition boundary or audio sample representation.

## Correctness evidence and limits

All **seven real-slot target links** pass the existing size/region guards, call-free **925-instruction `.ram_text`**, **60 B `.c1_text`**, target DSP budgets and **24 hot-function loop-call caps**. Original target probe contents for every preset name and field are identical in all seven variants. `cost-comparison.json`, per-result `checks.json`, `target-budget.log` and `target-inlining.log` retain exact evidence.

- The private insert oracle executes **160,000 variable-length blocks**, deterministic loud/bipolar inputs, all insert types and randomized controls/type/MIX transitions. It emits return status and every output sample: **11,188,896 bytes** are byte-identical for baseline, insert-only and combined. SHA-256 `06ba48b736fb77fe2ebd111eb469367550c653c95bcbe2b78ed02bccfc6bb5b6`. Existing `fx251_test` passes with identical output; `arena_test` passes with the required `-DARENA_STATS` instrumentation.
- The combined candidate passes `mix_split_test` for **seven scenes x eleven unit splits**, two actual host pthread cores with delayed scheduling (`CORES=2 CHAOS=1`). This includes multiple active inserts, per-core preset fades, drum sends/filter/slicer, texture, FREEZE, engine switches and live modulation. This is host ownership/output evidence, not target multicore timing or a ThreadSanitizer run.
- Actual UI tests pass including **20,000-frame fuzz**. All **79 captured framebuffers** match baseline for FFT-only, sample-scratch-only and the combined candidate. The existing fuzz macro emits a pre-existing unsequenced-seed warning; deterministic capture parity and the dedicated transaction/oracle comparisons carry the equivalence claim. No source warning was hidden by changing production tests.
- Existing `persistence_commit_test` runs its real transport refusal, backup begin/data/commit, cold reload and failed/corrupt/dropped side-write retry cases on baseline, sample-scratch and combined. `sequencing_phone_step_test` also passes. An early private driver incorrectly supplied a capture-directory argument to the settings test, which selected no cases; those invocations were discarded and the final runs use **no argument**, with all three PASS lines retained.
- The new isolated sample oracle uses production editor dispatch and a page-programmed fake NOR: **eight uploads**, all four slots, one/16-zone headers, malformed 17-zone rejection before retry, exact header/data bytes, scratch poisoning before requests, rescan after poisoning and a real backup restore all pass for original and proposed staging. Unrelated UI/editor commands are inert assertion hooks; external USB/DMA delivery is not simulated.
- Actual raw-versus-boot preset initialization checks **171 complete names/32-byte payloads**, scratch-capacity refusal, canaries and repeated restore. Canonical files match for both combined and combined-plus-relocation configurations, including the host pointer-size ABI. Target raw probe records independently retain the 36-byte target ABI.

Host compiler: pinned Zig **0.13.0**, `zig cc -UNDEBUG -O2 -Ibuild/gen -Ifirmware/src -pthread`, `-lm`; arena adds `-DARENA_STATS`, boot variant adds boot tables/presets defines. Commands/scripts and compile/result logs remain private. The initial uninstrumented arena compile failed for missing statistics declarations; the corrected instrumented final runs pass. The sample oracle initially needed inert definitions for unrelated command branches; final builds have them and retain the real upload/backup code.

This audit did **not** rerun the entire 200-render audio golden suite, build an installer, perform physical listening, measure hardware deadlines/stack high-water, or establish every optional build's link capacity. The insert byte oracle, effects tests and two-core split tests cover the changed audio-state semantics; full integrated release validation remains a gate after combining specialists' patches.

## Coordination with boot packing and final integration

The separate [boot-packing report](AMBIENT-HEADROOM-BOOT-PACKING.md) measures +1,008 B main RAM for its focused candidate or +4,000 B for its larger one. **Budget arithmetic only, not linked combinations with that report:** after this report's reductions, 13,008 B main margin would become **12,000 / 9,008 B**. If all presets are also moved to main, 6,848 B becomes **5,840 / 2,848 B**. Low RAM becomes 6,724 B free only when relocation is selected. The FFT pool recovery is counted once; the 8,192-byte reserve is still unavailable. Plan new resident code/state against the chosen regions before spending these margins.

Suggested integration sequence:

1. Review `combined.patch` as the concrete three-reduction candidate, or choose the RAM-buffer audit's FFT alternative and take only this report's independent `insert-union.patch` and `sample-scratch.patch`. Do not stack overlapping combined/component patches.
2. Keep preset relocation separate until the required low/main destination is known. `presets-main.patch` represents relocation alone; `combined-presets.patch` is the complete alternative including all three reductions. Regenerate assets privately/in the eventual normal integration build from the generator change; do not copy audit ELF/binaries into production.
3. Rebuild the actual integrated source with the existing slot and reserve; rerun raw/boot preset equality, actual UI captures, upload/backup error paths, insert oracle, effects/arena/render splits and full release audio/UI/storage tests. Existing ELF/source snapshots remain the .58 comparison even if HEAD has advanced.
4. Retain hardware gates for stack high-water, startup/restoration/watchdog, DMA and worst-case dual-core playback. No version bump, commit, push or publishing is part of this handoff.

Reproduce from PowerShell using the **existing frozen tree**, sequentially:

```powershell
python build/headroom-ambient/ram-state/candidate.py baseline
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 run.py baseline
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 ui-test.py baseline
python build/headroom-ambient/ram-state/candidate.py combined
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 run.py combined
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 test.py combined
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 ui-test.py combined
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/ram-state --exec python3 preset-split-test.py combined
python build/headroom-ambient/ram-state/compare.py
python build/headroom-ambient/ram-state/finalize-measurements.py
```

Other candidate modes are the names in the linked-result table; `candidate.py` restores saved original private files before applying a mode. `type-probe.py` reproduces unlinked ABI/layout evidence. `classify.py`, `padding.py`, `costs.py` and `verify-inputs.py` read retained evidence. **Do not rerun `prepare.py`** to reproduce the audit: it copies current shared files and would replace the frozen inputs. Saved result directories are authoritative; `source/build` contains the most recent build intermediates and need not match a subsequently reset private source. At handoff, private production-source inputs have been reset to their frozen baseline; review/apply the named patches in an isolated integration copy.

## Major symbol disposition appendix

Threshold: every sized named baseline object **>=128 B**, excluding overlapping compiler container aliases. The core-1 3,072-byte stack reservation and smaller initialized/boot/debug objects are accounted separately above. The complete unthresholded named inventory is `build/headroom-ambient/ram-state/named-state.csv`.

| Region | Symbol | Bytes | Ownership / disposition |
|---|---|---:|---|
| .bss | `trk` | 9088 | Live track/voice/pattern/lock state; ISR + guarded UI; retain; see layout/padding discussion |
| .bss | `DS_BASE` | 4114 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `FM6_EXP2` | 4100 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `abuf` | 4096 | Codec DMA double buffer; aligned 4; audio producer and hardware consumer; retain |
| .bss | `ins` | 4060 | Seven live insert channels; ISR/core-per-part; candidate union reduces exclusive state |
| .bss | `FONT_S_DATA` | 3952 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `main_scratch` | 3840 | Main-loop transient storage/FFT/OTA; 3840 B union; reuse only with nonoverlap proof |
| .bss | `proj_tmp` | 3840 | Legacy/current project and bank/backup/OTA staging; distinct from destination and I/O scratch; retain |
| .bss | `project_undo_saved` | 3816 | Persistent whole-project undo/redo; ordinary BSS .scratch; retain independently of song/autosave |
| .bss | `vis_frame` | 2400 | UI waveform/trail history; 2400 B union; persists across frames; retain |
| .bss | `vinyl` | 2128 | Stereo LOFI history, filters and coefficient cache; audio persistent; retain |
| .bss | `SINE` | 2050 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `midi_out_q` | 2048 | 512 packet ring; ISR producer / USB consumer; retain release/retrigger capacity |
| .bss | `ua_ring` | 2048 | 512 stereo-frame USB async audio ring; producer/consumer overlap; retain |
| .bss | `drums` | 2016 | Live drum voices and stable decoded factory parameters; audio persistent; retain |
| .bss | `UAC_FIR` | 1920 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `usr_zone` | 1792 | Validated/rebased user zones, four x sixteen; guarded publication; persisted zone ABI 28 B; retain |
| .bss | `dsu` | 1464 | Editable user synth-drum kits and runtime settings; persistent; retain |
| .bss | `ft_ev` | 1152 | 192 free-take events, six-byte time/duration/note/velocity records; bounded capture; retain |
| .bss | `FM6_FACTORY` | 1024 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `midi_sel_on` | 1024 | Already nibble-packed per-channel/note MIDI ownership; retain |
| .bss | `scope_buf` | 1024 | ISR-produced 512-sample history read by UI; retain separate from UI snapshot |
| .bss | `PITCH_OCT` | 768 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `undo` | 716 | Per-pattern undo state; retain alongside whole-project undo |
| .bss | `layer_screen_draw.tl` | 648 | Transient 27 tile records, rebuilt each draw; future typed main-scratch reuse candidate, unmeasured |
| .bss | `sx_frame` | 640 | 640-byte IRQ-fed SysEx receive frame, held through dispatch; cannot alias output/staging blindly |
| .bss | `ed_out` | 600 | 600-byte live reply frame; built around storage helpers and sent afterward; cannot alias st scratch |
| .bss | `fm6_eff` | 600 | Per-part effective/derived FM6 patch cache; used by audio; retain to avoid repeat work |
| .bss | `TANH_Q15` | 514 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `ENV_LIN` | 512 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `LFO_HZ_X100` | 512 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `LFO_INC` | 512 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `TIME_MS_X10` | 512 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `ed_smp_buf` | 512 | Main-loop 512-byte upload/header/backup staging; candidate main_scratch member |
| .bss | `lcd_fillbuf` | 480 | LCD fill transfer line, separate from live draw construction; keep DMA lifetime/alignment |
| .bss | `fm6_patch` | 468 | Mutable per-part FM6 patch records; distinct from factory bank and effective cache; retain |
| .bss | `FONT_S_OFF` | 448 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `smp_user_scan.zs` | 448 | 448-byte temporary validated zone copy, published IRQ-off; future typed cold scratch candidate, unmeasured |
| .bss | `ui` | 444 | Live UI navigation/cache/gesture state; persistent between frames; retain |
| .bss | `FM6_FINE` | 400 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `lim_dac` | 272 | DAC limiter history/state; persistent; retain separately from full-gain USB limiter |
| .bss | `lim_usb` | 272 | Full-gain USB limiter history/state; persistent; retain separately from DAC limiter |
| .bss | `ed_w.10` | 260 | Persistent editor WATCH values/timestamps/signatures; compiler scalarized struct; not duplicate full objects |
| .bss | `ed_w.9` | 260 | Persistent editor WATCH values/timestamps/signatures; compiler scalarized struct; not duplicate full objects |
| .bss | `CUTOFF_HZ` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `DECAY_K` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `ENV_EXP` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `FM6_DETUNE` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `LEVEL_DB_X10` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `LEVEL_Q12` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `SVF_G` | 256 | Boot-restored immutable table: persistent after boot, ordinary SRAM; do not union with scratch or decode in playback |
| .bss | `ed_w.11` | 256 | Persistent editor WATCH values/timestamps/signatures; compiler scalarized struct; not duplicate full objects |
| .bss | `midi_in_q` | 256 | 64 MIDI packets, USB/UART producers and audio consumer; retain ownership/barriers |
| .bss | `midi_scale_held` | 256 | 64 four-byte scale note ownership entries; release must use press mapping; retain |
| .bss | `sx_out_q` | 256 | 64 SysEx USB packets, foreground producer / endpoint consumer; retain |
| .bss | `usb_out` | 256 | Full-gain USB block stereo output; audio/USB tap lifetime; retain |
| .bss | `vs` | 236 | Persistent visualizer smoothing, bars and peak holds; retain |
| .bss | `ep4tx` | 196 | Asynchronous USB audio DMA packet, aligned 4; remains live until consumed; retain |
| .bss | `tx` | 196 | Texture synthesis/noise/filter/user playback state; audio persistent; retain |
| .bss | `ed_handle.r` | 192 | 192-byte user preset upload parse/canonicalization record; remains live through bank staging/storage; do not alias storage scratch |
| .bss | `punch` | 188 | Punch modes/crossfade state; cold BSS initialization already integrated; retain |
| .bss | `cfg_rate` | 178 | Mutable USB configuration descriptor; EP0 references survive helper return; retain |
| .bss | `sl` | 176 | Four slicer transport/gate/stutter state records; audio persistent; retain |
| .bss | `tflt` | 160 | Track filter state, separate per-track channels; audio persistent; retain |
| .bss | `song` | 140 | Live globals/transport/meter state; ISR/UI ownership; retain saved conversion boundaries |
| .bss | `fm1_in` | 136 | Panel sampled input and counters shared with ISR/main; retain |
| .bss | `rs` | 136 | USB ASRC/FIR history and phase; audio persistent; retain |
| .bss | `ds_buf` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `mix_block.ol` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `mix_block.orr` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `mix_block.ul` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `mix_block.ur` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `punch_primary.kl` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `punch_primary.kr` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `rev_l` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `rev_r` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `sl_dbuf` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `stq` | 128 | Sixteen strum queue entries, on/track/note/velocity plus 32-bit sample countdown; persistent pending events; retain |
| .bss | `texture_mix.tl` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `texture_mix.tr` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `um_ring` | 128 | 128-byte UART RX DMA ring aligned 16, content-sentinel protocol; retain |
| .bss | `vis_tap` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `wet_l` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .bss | `wet_r` | 128 | Audio block scratch/return buffer; simultaneous pipeline roles; no lifetime sharing proven |
| .lowram | `rev_line` | 17368 | 8684 int16 reverb samples; audio persistent; low placement for capacity, no algorithm reduction |
| .lowram | `acc` | 1280 | Two per-core mix accumulators, 5 x 32 int32 each; concurrent; retain separate rows |
| .lowram | `PHYS_PRESETS` | 1008 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `DIGITAL_PRESETS` | 864 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `ANALOG_PRESETS` | 792 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `LOFI_PRESETS` | 720 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `drum_buf` | 512 | Four simultaneous drum/filter/send blocks; audio pipeline; retain |
| .lowram | `TRIO_PRESETS` | 504 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `NOISE_PRESETS` | 396 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `SMP_PRESET_TABLE` | 396 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `PHASE_PRESETS` | 360 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `DRAWBAR_PRESETS` | 288 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `FM6_PRESETS` | 288 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `GRAIN_PRESETS` | 288 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .lowram | `mix_part.pnew` | 284 | Two per-core 71 x int16 parameter snapshots; concurrent; retain |
| .lowram | `part_buf` | 256 | Two per-core mono render blocks; concurrent; retain separate rows |
| .lowram | `FORMANT_PRESETS` | 252 | Boot-restored factory presets; ordinary 32-bit pointer ABI; low-to-main relocation candidate, not RAM reduction |
| .noinit | `proj_slot` | 15264 | Four noinit sections, 3816 B each; survive reset/UBOOT; preserve storage ABI |
| .pool | `dly_buf` | 131072 | 65536 int16 delay samples; persistent musical delay; reducing changes available time/sound |
| .pool | `punch_ring` | 65536 | 32768 int16 stereo punch history; persistent alongside ordinary delay and slicer; retain |
| .pool | `cv_px` | 59520 | 240 x 124 RGB565 UI canvas; LCD transfer lifetime; retain dimensions/format |
| .pool | `sl_buf` | 32768 | Four x 4096 int16 stutter histories; all tracks may run simultaneously; retain |
| .pool | `eng_arena` | 25776 | Three 8592-byte engine arenas, engine-exclusive within each part; already shared; retain |
| .pool | `cho_buf` | 4096 | 2048 int16 chorus samples; persistent alongside delay/reverb; retain |
| .pool | `autosave_buf` | 3816 | Current save/import/undo exchange project; live concurrently with staging/history; retain |
| .pool | `song_keep` | 3816 | Arrangement original working-project snapshot; persistent through playback; retain |
| .pool | `vis_sg` | 3456 | 72 x 48 spectrogram history, retained across style changes; cannot alias trails unchanged |
| .pool | `rev_storage` | 1996 | Existing 1996-byte diffuser/spring union; already shares exclusive models; retain |
| .pool | `vis_im` | 1024 | Ephemeral FFT imaginary vector; candidate reuses existing main_scratch.vis[1] |
