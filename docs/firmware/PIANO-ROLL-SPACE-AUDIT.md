# Piano roll flash space audit

Measured 2026-10-21 using a private snapshot in `build/piano-space-audit`.
The investigation itself left production sources, shared generated headers/build outputs, Git refs/index and hardware untouched. **Parent integration has since applied all four accepted candidates**, including the fixture changes and subsequent target rebuild. The measurements and then-outstanding test observations below remain the frozen audit record, not current release status. No engine, sample, UI mode, reverb, or other feature was removed; no sample decompression changes. See [current optimization accounting](OPTIMIZATION.md) and [upstream suggestions](UPSTREAM-OPTIMIZATION-CANDIDATES.md).

## Ranked results

| Candidate | Target app bytes | Saved vs snapshot | Free in 581,564-byte slot | Risk / merge cost |
|---|---:|---:|---:|---|
| Snapshot baseline | 580,724 | — | 840 | Includes in-flight UI edits |
| **1. Drum state to BSS** | **578,692** | **2,032** | **2,872** | Low; drums.c only plus mechanical host fixture cache resets |
| **2. Limiter delay state to BSS** | **579,944** | **780** | **1,620** | Low/moderate; tiny fx.c change, adds one check per limiter block |
| **Both** | **577,928** | **2,796** | **3,636** | Small, reversible; savings are not exactly additive |

The packaged 2.7.1.36 image copied at audit start is 580,304 bytes (1,260 free). The private current-source snapshot rebuilt 420 bytes larger, so all candidate deltas above use **580,724**, not the packaged image. Do not subtract the measured savings from the old package and call that a measured result. New MIDI, wet-color, and later piano roll edits require rebuilding after application.

### 1. Separate drum factory-set cache

`patches/01-drums-bss.patch` moves the sole nonzero initialized field (`set = -2`) out of the 2,020-byte `drums` object into a two-byte `drum_factory_set` cache. The remaining drum runtime state starts zeroed in BSS. The lazy factory PERC lookup retains its -2 unknown / -1 absent / index semantics. Removing the old field also removes padding from the drum object.

The target .data shrinks 2,016 bytes and .text shrinks 16 bytes: **2,032 bytes measured image saving**. Static RAM occupancy stays 85,688 bytes due to section alignment. No new work is added to the render path. `drums.set` is internal, never persisted or sent over the wire. Eight existing host test files explicitly reset it; `patches/01-drums-test-fixtures.patch` supplies their mechanical rename. Merge cost: one production file and fixture substitutions. Prefer this first.

### 2. Zero-initialize limiter delay buffers

`patches/02-limiter-bss.patch` removes nonzero static initializers from `lim_dac`/`lim_usb`, moving two 272-byte objects to BSS. At `lim_block` entry a zero envelope selects the former initial control values in local variables: `e=LIM_T`, `g0=32768`. Every completed call stores an envelope >= LIM_T; thus zero identifies fresh state. Existing explicit USB reset behavior is retained.

Measured .data shrink is 556 bytes including compiler layout effects; .text shrinks 224 bytes overall. `lim_block` itself grows from 324 to 354 bytes; `mix_block` shrinks from 14,620 to 14,374 bytes. The result is **780 bytes of target image saving**, not a source estimate. Static main RAM stays 85,688 bytes. Merge cost: two nearby hunks in fx.c, which another chat currently owns; parent should coordinate application. Risk: changed first-use representation and a per-block conditional; no hardware timing result is claimed.

`patches/combined.patch` contains both production candidates. Combined savings are 16 bytes below the sum of individual savings because target compiler layout changes interact.

## Sections (decimal bytes)

| Section | Baseline | Drum only | Limiter only | Both |
|---|---:|---:|---:|---:|
| .text (code + ROM metadata/assets) | 574,412 | 574,396 | 574,188 | 574,188 |
| .ram_text | 2,956 | 2,956 | 2,956 | 2,956 |
| .c1_text | 60 | 60 | 60 | 60 |
| .data | 3,296 | 1,280 | 2,740 | 724 |
| .bss | 82,392 | 84,408 | 82,936 | 84,952 |
| .lowram | 22,772 | 22,772 | 22,772 | 22,772 |
| .pool | 332,876 | 332,876 | 332,876 | 332,876 |

Image bytes include linker alignment; summing named sections can differ. Main RAM use is `_bss_end - 0x01c08000`, including gaps, rather than just .data+.bss sizes. Pool headroom remains 11,188 bytes. Existing build checks passed for baseline and every candidate: valid entry, no RAM-text calls, no forbidden ROM reference, flash/RAM/pool bounds.

## Evidence and reproduction

All paths below are relative to `build/piano-space-audit`:

- `original/`: immutable firmware snapshot; `snapshot-sha256.json`: its per-file SHA256 manifest.
- `tools/`, `tests/`, `build/gen/`: private tools/tests and copied generated assets; no asset generator was invoked against the live tree.
- `packaged-felucca.{bin,elf,dis}`, `packaged-symbols.txt`: copied packaged baseline evidence.
- `results/{baseline,drums-bss,limiter-bss,combined}/`: actual target bin/ELF/disassembly, symbol table, sections and check log for each case.
- `run.py`: invokes only private `build.build_app()` and `build.check()`; no loader/package, generators, downloads, or SDK writes.
- `drums_candidate.py`, `limiter_candidate.py`: apply transformations only to private firmware; restore the relevant file from `original/src/` before rerunning either transform.
- `verify.py`: compiles private tests using the existing Zig compiler, with both Zig caches redirected inside the audit directory.

PowerShell target command (all outputs stay private):

```powershell
wsl -d Ubuntu --exec python3 /mnt/c/code/sloop-fm1/build/piano-space-audit/run.py baseline
```

For exact baseline reproduction first copy `original/src/drums.c`, `original/src/fx.c`, `original/src/params.c`, and `original/src/libc.c` back to `firmware/src/` **inside the private directory**. `run.py` reads the frozen private assets. Then run `python drums_candidate.py` for drum only, `python limiter_candidate.py` for limiter only, or both, and invoke `run.py` with the desired result label. Existing target toolchain: `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`; flags are the private copied build.py defaults (`-Oz`, single compilation unit, FELUCCA_BOOT_TABLES=1, FELUCCA_SLICE=0). SDK packaging is unnecessary for image measurement.

Host commands from the private directory through WSL:

```sh
python3 verify.py baseline-audio
python3 verify.py combined-audio
python3 additional_checks.py
```

For `baseline-audio`, restore the four original production files above and replace private test fixture `drum_factory_set` references with `drums.set`; for `combined-audio`, apply both candidates (the drum transform also updates those test resets). The included `tests/audit_regress.c` is a private variant of existing regress.c: it skips timed CPU jobs and returns after the existing audio, health, determinism, and voice checks. It writes a fresh baseline golden file only under `results/baseline-audio/`; shared goldens/budgets are never updated. `tests/limiter_equivalence.c` compares the original lim_block body directly with the candidate. Run that test only on candidate sources. `build/host/` inside the audit must exist for swing-grid image output. No full-suite or target hardware equivalence is implied.

## Other avenues inspected

The packaged symbol table and disassembly show 303,054 bytes of sample ROM, already compact preset tables, packed BANK records, and 516 bytes of page metadata. Packing the page table has at most 43 bytes of gross padding to remove and introduces unaligned pointers; descriptor packing would affect every engine and accessor. These are lower-value changes than eliminating initialized runtime buffers, and are **unmeasured estimates**, not recommended patches. Fonts were not changed.

Two target experiments were rejected: `params-noinline` (eight cold parameter helpers) grew the image **48 bytes**; `strings-noinline` (five text helpers) saved **zero bytes**. Their actual ELF/disassembly/sections remain under `results/`. No broad compiler flag changes, DSP rewrites, or upstream-facing metadata ABI changes are proposed.

## Validation results

- **182 baseline-generated golden audio renders match exactly** on combined candidates: zero changed/missing renders, zero health failures, zero voice/routing failures, zero crashes. Includes every engine/preset, drum kit, voice modes, sends, four-track mix, slicer and repeat determinism. All seven voice/routing checks pass, including 14,135 randomized MIDI/key events across three rounds.
- `limiter_equivalence`: **100,000 blocks pass**, comparing the original algorithm and candidate output samples **and entire state** after each call. Includes n=0..CTL, signed overload up to +/-4,194,304, silence, and repeated fresh resets. This supplies stronger first-use coverage than the regular tests that explicitly initialize limiter controls.
- Existing `drumkit_test`, `dsyn_test`, `punch_test`, and `swing_grid_test`: **pass** on baseline and combined candidates. Swing initially lacked its private output directory; rerun passed after creating it.
- Existing `fx251_test`: **same single failure on baseline and combined candidate**, the assertion `MIDI FX CH: G5 the 16th effect; an FM-1 key taking over keeps it; IN = CLOCK ignores it; IN = CH n leaves it`. All other assertions match. This audit did not change MIDI routing or investigate that concurrent-work baseline failure.
- Baseline and combined **target_budget.py pass**. All measured render-loop costs are unchanged, including drums_mix (3,658), and audio IRQ (256). The limiter is not included in that script's function list: its extra envelope check is per block, and its cost is not covered by this unchanged-loop claim. Full wall-clock/hardware CPU timing was not run.

Detailed logs: `results/baseline-audio/*.log`, `results/combined-audio/*.log`, and `results/{baseline,combined}/target-budget.log`. No target hardware playback/flash test or complete repository suite was run. Parent should apply the drum patch first, include its fixture rename patch, and rebuild against final concurrent sources before deciding whether the extra limiter change is needed.

## Follow-up: additional savings and name-to-ID lookups

The follow-up continues against the **same frozen 580,724-byte baseline**, not changing concurrent production sources.

| Additional candidate | App bytes alone | Saved alone | Main RAM | Merge / maintenance risk |
|---|---:|---:|---:|---|
| **3. Texture state to BSS** | **580,276** | **448** | 85,688 | One line in fx.c; existing reset seeds noise |
| **4. Immutable preset browser indices** | **580,004** | **720** | **85,528** | ui.c table change; update indices when engine preset order changes |
| **All four candidates** | **577,040** | **3,684 total** | **85,528** | **4,524 bytes flash free**, 160 bytes less main RAM |

All four save **888 bytes more** than the first combined pair's 577,928 bytes. Do not add individual savings: the compiler's global layout/optimization interactions are material. Patch files: `patches/03-texture-bss.patch`, `patches/04-bank-index-rom.patch`, and `patches/all-four.patch`. Previous `combined.patch` still contains only candidates 1+2. `results/summary.json` records image sizes, slot headroom, and SHA256 for every target measurement.

### Texture reset already owns the seed

`tx` was a 196-byte initialized object solely because `.rnd=0x2545F491`. Its existing `texture_reset` already supplies that seed if rnd is zero. At boot, tx.src=0 and tx.g=0; texture_mix returns without generation while OFF, and resets any changed source before it can play. Thus zero initialization avoids redundant ROM state without adding a runtime branch. No caller outside texture_mix invokes texture_gen. Direct reset behavior and later PRNG continuity are retained.

Measured sections: .text 574,172 (-240), .data 3,088 (-208), .bss 82,600 (+208). Differences beyond the 196-byte object are target compiler alignment/global-layout effects, not more buffers removed. All four use .text 573,500, .data 524, .bss 85,000; .ram_text/.c1_text/.lowram/.pool remain unchanged.

### Browser names can be resolved once, before shipping

The old BANK contained 153 five-byte records: kind/engine nibbles plus a name pointer. bank_resolve searched each engine's presets by name, populated a 153-byte RAM index array and set a ready flag. The candidate retains BANK order, kind and engine exactly, reduces BANK to 153 one-byte records, and makes the same resolved indices a separate 153-byte **const** bank_pi table. Names remain in the engines' canonical preset tables and in comments on browser records. It eliminates runtime search and ready checks. A no-op bank_resolve remains for existing host fixtures and compiles away. Existing uint8_t index consumers and the 0xFF fallback remain valid.

This is a measured 720-byte flash / 160-byte RAM saving. The original resolver was executed in the private baseline host fixture to generate `bank-resolved.txt`; all 153 entries resolved successfully. `tests/bank_equivalence.c` checks each compact entry's original engine, index, canonical preset name, base_at/base_pos round trip and favourite key. It passes. Existing ui_pages_test also passes on baseline and candidate; its one failure diagnostic referenced removed BANK.name, so `patches/04-bank-ui-fixture.patch` changes that diagnostic to an index-entry description. This does not change assertions.

Merge cost is moderate compared with the BSS changes: the browser table has many small record edits. **Preset reordering is the maintenance risk**. The current name resolver tolerates engine preset reordering automatically; a resolved table must be regenerated/updated and checked against original name expectations whenever presets change. Preserve browser row order because favourite indices and Program Change depend on it. Parent should retain equivalent name/index validation if adopting this candidate. The private mapping generator (`bank_dump.py`) requires original ui.c; the candidate transformation (`bank_candidate.py`) starts from the frozen original ui.c and the recorded mapping.

### How many other such cases?

The source scan found **113 static const character-pointer array declarations**, but most are display enums, not redundant name-to-ID searches. Those are not automatically equivalent savings: string literals are already shared by the compiler, and replacing them can add accessor code or duplicate text.

Beyond BANK, concrete related lookup sites are:

| Lookup | Entries / scope | Assessment |
|---|---|---|
| ICON_MAP: parameter label to icon | 137 entries, 685-byte packed map | Largest remaining name map. Direct IDs would require coordinated descriptor/caller changes; metadata already packed. **Unmeasured**, broader merge cost. |
| WAVE_ICON: waveform name to icon | 13 entries | Could map canonical enum values; engines have different value lists. **Unmeasured**, must preserve unknown-name fallback and dynamic descriptors. |
| engine_icon: engine name to icon | 11 entries | Small candidate for engine-ID lookup; includes DRUM and generic fallback. **Unmeasured**. |
| cc74_slot: engine name to EDIT slot | 10 rows | Good small direct-engine-index candidate; preserve descriptor range/name-list checks. In seq.c, which concurrent MIDI work owns. **Unmeasured**. |
| CC resonance: scan engine labels for RES/Q | Up to 8 descriptors per engine | Similar direct slot metadata opportunity, but changing descriptors/canonical engine ordering needs tests. **Unmeasured**. |
| drum_set: find PERC sample set by name | One lazy lookup | A generated PERC ID could remove lookup/cache. Must derive it from asset generation and preserve missing-set semantics. **Unmeasured**. |

`ui_draw.c` also compares cached displayed strings; that is redraw change detection and cannot simply become a preset ID. No additional blanket conversion is proposed. Of the exact name-search cases, BANK is the clear largest low-scope win; the rest require measurements before ranking.

The experiment relaxing noinline on 18 tiny UI helpers saved **32 bytes** while editing multiple UI files. It is recorded under `results/tiny-ui-inline` and rejected as poor merge cost for the benefit. Those changes are absent from all-four.patch and the current private candidate sources.

### Additional validation

- `check_more.py more-baseline texture_hash ui_pages_test` records baseline texture hashes and UI checks; `check_more.py more-candidates texture_hash bank_equivalence ui_pages_test` checks candidates. Use the existing Zig compiler/caches as configured in the script.
- `texture_hash` exercises 35 source/play/always/tone/movement/rate/duck phases of 600 blocks each: **all audio hashes match**. PRNG/source state matches after first activation. Before activation the unused seed is intentionally zero instead of initialized; no audio differs.
- All 153 original browser mappings, browser positions and favourite keys pass; existing UI suite passes baseline and candidates.
- All-four target image safety checks and static target loop budgets pass (`results/all-four/checks.txt`, `target-budget.log`).
- Expanded audio/FX test results are recorded under `results/all-four-audio`; see final log status below.

Final expanded test status: **182 golden renders match, zero health/voice/crash failures**; drumkit_test, dsyn_test, punch_test, swing_grid_test, and 100,000-block limiter_equivalence all pass. fx251_test retains the **same single baseline MIDI FX-channel assertion failure**, with no new failing assertion. The additional texture assertions in that suite pass. No shared sources, test runners, assets, build outputs, Git state or hardware were modified.

## Round two: another 4,448 bytes, after the first four were applied

A new immutable source snapshot lives under `build/piano-space-audit/round2/original`, with its own SHA256 manifest. Earlier round-one evidence was not overwritten. Parent's copied package was 580,396 bytes; the in-flight source snapshot rebuilt **580,668 bytes**, main static RAM **85,592**. This difference reflects concurrent changes; all round-two savings are measured against **580,668**, not the parent's earlier package. The later full ACTIONS source is not in this snapshot. Do not claim its final headroom until parent rebuilds it.

| Ranked candidate | Target image | Measured new saving | Main static RAM | Main risk / merge cost |
|---|---:|---:|---:|---|
| **1. Boot-only bit packing + FIR + FM6 factory** | **577,324** | **3,344 bytes** | **89,048** | Startup decoder/generator change; +3,456 RAM, no render-path decode |
| **2. Byte-sized descriptor bounds/defaults** | **579,564** | **1,104 bytes** | **85,592** | One core.h type edit; future values must fit the explicit ranges |
| **Both (recommended set)** | **576,220** | **4,448 bytes** | **89,048** | 5,344 bytes flash free on this snapshot; 9,256 bytes main RAM free |

Final unapplied patches, relative to the repository:

- `build/piano-space-audit/round2/patches/01-descriptor-bytes.patch`
- `build/piano-space-audit/round2/patches/02-boot-bitpacking-and-fir.patch`
- **`build/piano-space-audit/round2/patches/combined-4448.patch`** (243 lines, both candidates and focused test updates)

Apply source/tool/test changes, then run normal asset generation/build: **do not copy the private generated headers or binaries into the live checkout**. The final patch supersedes the intermediate `combined-4096.patch`. Parent currently owns core.h and should merge its small descriptor hunk alongside its own changes.

### Boot-only compression, preserving all coefficients and patch bytes

The existing startup table infrastructure already expands 20,122 immutable bytes into RAM before audio/USB/second-core startup. Its LZ4 payload is 8,212 bytes. The candidate adds a direct bit-packed residual alternative: try finite-difference orders 0..4, encode residuals relative to their minimum using only the required bits, and select it only when smaller than the existing LZ4 option. `boot_table_t.bits` occupies former padding; zero keeps the original LZ4 mode. The new bit reader validates bit width and exact encoded length before writing. The existing modular integration reconstructs the original values.

This shrinks payloads for the numeric synthesis tables (not the fonts) and makes the 512-byte LFO_INC worth expanding once at startup. Existing font data/offset/metric payloads and DS_BASE payload are **unchanged**. The audio sample data is untouched. No per-hit/sample decompression is introduced.

Separately, the 1,920-byte USB FIR table is smoothly varying by phase. A new `tools/pack_uac_fir.py` stores each column's first two coefficients plus packed second differences: **351 bytes**. `uac_fir_init` restores the original 80x12 int16_t table once during startup. The USB resampler loops and coefficient access stay unchanged. The generated init hook is excluded from audio-only host harnesses, which do not include USB.

Finally, including `felucca_fm6.h` in the existing table packer compresses its 1,024-byte FM6_FACTORY to **657 bytes** and expands it once at boot, before patch loading. FM6_INIT remains untouched. The generator strips C comments before numeric parsing; canonical uncompressed declarations remain in the fallback branches. No factory patch is removed or changed.

Final packed-table RAM is 21,658 bytes, payload 7,696 bytes; FIR adds 1,920 RAM and 351 payload bytes separately. The generator's existing 24,000-byte boot-table RAM guard now counts FIR too: total **23,578 / 24,000**. Main static RAM increases **3,456 bytes** (LFO_INC 512 + FIR 1,920 + FM6 factory 1,024). Pool and low RAM are unchanged. Startup code/cost increases; **no hardware startup duration, watchdog margin, or cycle timing has been measured**. Existing boot order confirms restoration precedes audio, USB start and the second core.

### Compact descriptors without packed/unaligned pointers

`param_desc_t` was 20 bytes on the target: three pointer fields, uint8 format, three int16 bounds/defaults, and padding. Changing min to int8_t, max to uint8_t, def to int8_t makes it **16 bytes**, with ordinary aligned pointers. Parameter values in tracks/projects/wire records remain int16_t; no serialized structure changes. Current descriptor bounds are **-64..240**, defaults **-12..127**, so those explicit byte types preserve all values. Integer promotion of uint8 max remains signed int on the target/host, preserving comparisons and subtraction.

`descriptor_oracle` visits **12,428 descriptors** (track/global/insert/sync/record/drum plus every engine's dynamic descriptors across 128 mode settings), hashes fields and every in-range formatted value/unit, and matches baseline exactly: **7ed098ab6db85661**. Future descriptor extensions must keep min/default in -128..127 and max in 0..255, or widen the types and accept the flash cost. Existing target builds emitted no narrowing diagnostics.

### Measured sections

| Section | New baseline | Final combined |
|---|---:|---:|
| .text (code + ROM data) | 577,128 | 572,680 |
| .ram_text | 2,956 | 2,956 |
| .c1_text | 60 | 60 |
| .data | 524 | 524 |
| .bss | 85,064 | 88,520 |
| .lowram | 22,772 | 22,772 |
| .pool | 332,876 | 332,876 |

Actual ELF/disassembly/sections/symbols/check logs are in `round2/results/{baseline,descriptor-bytes,final-boot-only,final-boot-and-descriptors}`. `results/summary.json` supplies sizes and hashes. The package image is not the sum of sections when linker alignment introduces gaps.

### Validation and reproduction

- Every byte of **21,658 restored generic table bytes** matches the raw-header oracle, including all 1,024 FM6 factory bytes; malformed LZ4 and new bitstream width/length tests pass.
- All **960 FIR coefficients** match the original table after first and repeated initialization. Existing USB resampler test passes with **actual boot-restored FIR coefficients**, including its 48 kHz response/ring/packet checks.
- **182 audio golden renders match the fresh round-two baseline**, with no health, voice/routing, or crash failures. The final run compiles with `FELUCCA_BOOT_TABLES=1`, so it renders using the actual reconstructed numeric/FM6 tables, not only fallback constants. Final log: `round2/results/final-boot-audio/regress.log`.
- Existing drumkit, dsyn, fx251, punch and swing tests pass in native mode; the previously reported MIDI baseline failure is fixed in this newer snapshot. Actual-boot mode also passes drumkit/fx251/punch/swing. dsyn initially failed because its factory oracle ran before host_tracks_init; the included test hunk initializes boot tables first when that mode is enabled. The corrected **dsyn boot-mode run passes** (`results/boot-validation/dsyn_boot_test.log`).
- Final target image checks and static DSP loop budgets pass. No sample-loop work is added. No hardware flash or complete repository test suite was run.
- Private `reproduce.py baseline` restores original firmware/packer and rebuilds recovered raw fallback headers; `reproduce.py candidate` restores the candidate files and regenerates/rebuilds with the new packer. Both write only under round2. Use WSL Ubuntu and the same existing JieLi path as round one. `boot_verify.py` checks restored table/FIR/USB/drum behavior; `final_audio.py` compiles and runs the actual-boot audio regression subset. `check_more.py candidate-metadata descriptor_oracle fir_boot_test` records the field/format and coefficient comparisons.

Rejected/secondary round-two experiments: marking all UI helpers cold **grew 592 bytes**; splitting two layer-render branches saved only **80 bytes** across a large relocation; packing immutable preset records saved only **96 bytes** and introduced unaligned pointer layout; outlining SVF coefficients saved **192 bytes** but adds block calls in DSP code; a nullable empty-preset record saved **192 bytes** but touches record/backup semantics for a small return. None of these is in the final patch. DS_BASE byte transposition worsened compression (2,831 to 3,530 bytes), and was rejected before any target code change.

Round-two reproduction confirmation: baseline and final candidate reproduce **byte-for-byte identical SHA256 hashes**, not only matching sizes.

## Round three: 976-byte UI/control helper saving (applied by parent)

A separate frozen snapshot and copied assets are in `build/piano-space-audit/round3/{original,original-tools,original-gen}`. The copied package and rebuilt source baseline both measure **581,300 bytes**, main RAM **89,064**. This snapshot includes full note/hit ACTIONS but predates the parent's final pitch-grid and OFF edits. Earlier rounds remain immutable evidence.

| Candidate | Target bytes | Measured saving | Main RAM |
|---|---:|---:|---:|
| Baseline | 581,300 | — | 89,064 |
| **Shared UI clamp (recommended, now applied)** | **580,324** | **976 bytes** | **89,064** |

Patch: **`build/piano-space-audit/round3/patches/01-ui-clamp-976.patch`**. It adds one `UI_SMALL` noinline `ui_clamp` wrapper in ui.c, with the same int32 input/output types and original clamp implementation. A scoped macro routes the following UI includes through that shared helper; felucca.c undefines the macro immediately after ui_input.c. Normal per-sample DSP code is compiled earlier and keeps its forced-inline clamp. There is no startup-table, persistent state, wire-format, asset, or RAM change. The selected UI/control paths add a function call instead of repeated clamp instruction sequences. Musical-starter preview control can use that helper at note/block boundaries; no per-sample decoder or sample-loop work is introduced.

The patch is two small hunks and was handed to the parent promptly when the pitch-grid integration overflowed by 136 bytes. Parent has since reported applying it and shipping build **2.7.1.38 at 580,708 bytes, 856 flash bytes free, 89,064 main RAM** with the true pitch grid and LOFI OFF. Those shipped integration numbers are **parent-reported**, not this audit's frozen target measurement. Parent also reports all eleven UI/MIDI suites passing, including 20,000-frame fuzz.

### Validation

- Target image safety checks and static DSP budgets pass for the measured clamp candidate. No hardware timing/flash test was performed here.
- **All 79 baseline UI PPM frames match byte-for-byte**; `round3/results/ui-frame-sha256.json` records both SHA256 sets. Additional piano-roll screenshots from its dedicated suite account for extra candidate output files and are not confused with baseline comparison frames.
- Existing **piano_roll_test, piano_note_selection_test, piano_notes_review_test, piano_actions_ui_test, note_chain_actions_test, drum_hit_actions_test, and drum_fx_ui_test pass**.
- The frozen baseline and candidate each had the identical single ui_pages_test failure: `LOFI selector and amounts reach the master-bus parameters`. Parent subsequently corrected that obsolete max=3 expectation to max=4 for OFF and reports the updated suite passes. No new candidate assertion failed.
- The private source tree is left with **only ui.c and felucca.c differing from the round-three firmware snapshot**, matching the recommended patch. Production sources, shared generated headers/build outputs, Git state, test runners and hardware were untouched by this audit.

Evidence: `round3/results/{baseline,ui-clamp-shared}` contain target ELF/bin/disassembly/symbols/sections/check logs; `round3/results/{baseline-ui,clamp-ui}` contain host logs and rendered frames. `round3/results/summary.json` records target sizes and hashes. `clamp_candidate.py` applies only the frozen private change. To reproduce baseline, restore original/src/ui.c and original/src/felucca.c into private firmware/src and use copied original-gen assets, then run private run.py. To reproduce the candidate, run clamp_candidate.py, then `wsl -d Ubuntu --exec python3 /mnt/c/code/sloop-fm1/build/piano-space-audit/round3/run.py ui-clamp-reproduced`. Existing JieLi paths/flags are unchanged. `check_more.py` compiles each named focused test with isolated outputs and the existing private Zig caches.

### Rejected small or unsuccessful experiments

- Sharing the gauge RATIO formula added **zero savings** beyond clamp; excluded.
- Extending the clamp macro into later project/editor/main includes saved **48 additional bytes**; rejected because the wider include scope is not justified by that small benefit. The recommended patch stops at ui_input.c.
- Explicit noinline attributes on fourteen drawing helpers saved **zero bytes**.
- A broad noinline experiment across starter/rhythm/hit helpers overflowed XIP by **1,736 bytes** and was rejected; no successful target image or positive saving is claimed for it.
- A CC74 direct-engine-index table measured **96 bytes saved** alone (581,204 bytes), with original-name-resolver equivalence across every engine, all 128 CC values, and seq2_test passing. **Parent explicitly declined this candidate** because the small saving does not justify MIDI maintenance/coupling. Its private patch/results are experimental evidence only, not a recommendation or part of the delivered clamp patch. It was removed from the final private source tree. No further work is planned on it.

Round three is complete. No larger independently validated UI/control candidate emerged beyond the shared clamp; the rejected experiments are not presented as additional available headroom.
