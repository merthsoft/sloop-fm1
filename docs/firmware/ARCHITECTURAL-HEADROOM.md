# Architectural headroom investigation

Status: investigation complete; the preset representation and aligned slot capacity are now integrated in the parent source. The agent experiments left production untouched; parent integration rebuilt to **580,312/584,336 bytes (4,024 free)**. Main RAM is **89,080/98,304**, low RAM **28,280/29,496**, pool **332,876/344,064**. Actual restored records, 182 audio goldens, 29 native/WASM profiles, UI/persistence and target DSP checks pass. Hardware SPL boot and startup timing remain unverified. Frozen evidence lives under `build/headroom-study/`; parent checks under `build/integration-40/` (ignored build artifacts).

## Findings and priority

We can recover a useful first chunk of space without dropping features, reducing sample quality, or decoding audio assets during playback. Two measured candidates combine to **5,352 bytes of additional usable flash**: 2,772 bytes of unused capacity already inside the existing update boundary, plus 2,580 bytes saved by storing sparse factory preset metadata and restoring it once at boot. They have different costs: the capacity adjustment needs package/boot compatibility validation; the preset representation consumes 5,508 bytes of spare low RAM and adds one cold-start operation.

These are the start of a larger restructuring, not evidence that arbitrary tens of kilobytes are available. UI/control code is the largest realistic remaining code target. Factory audio dominates the image, but the easy lossless direct-read sharing is already done, and real-time decompression remains excluded.

| Priority | Change | Benefit | Evidence / cost |
|---|---|---:|---|
| 1 | Use trailing space inside the existing encrypted app region | **+2,772 B capacity** | Exact SDK/package arithmetic and private maximum-size package/loader tests; no new physical erase range. Real SPL boot of the changed directory still needs hardware verification. |
| 2 | Sparse factory preset metadata, expanded before audio starts | **2,580 B net saved** | Actual target link, all 153 preset names and field bytes checked, 182 actual-restoration audio goldens unchanged. +5,508 B low RAM; no new main/pool buffer. |
| 3 | Shared UI view models, field descriptors and event routing | **4–10 KiB planning target, not measured** | UI functions currently ~72 KiB. Requires one end-to-end converted family and target comparisons before a savings claim. Medium development/upstream-merge cost. |
| 4 | Explicit lifetime sharing for old-project conversion scratch | **Up to 2,584 B RAM, not flash** | Symbol size measured; alias/lifetime safety not yet proven. Could fund another boot-restored metadata/code candidate. |
| 5 | Reclaim 0x93000..0x97000 as app flash | **At most 16 KiB gross** | Unproven partition redesign, blocked on SPL update-record scan, old-loader migration, VM descriptors and SFC plaintext boundary. Not recommended for the current package. |
| Rejected | More direct contiguous audio deduplication | Negligible | 40 unique ADPCM streams already service 51 zones; current reuse avoids 77,064 B of duplicated zone storage. No complete remaining stream is a substring of another; small boundary overlaps total at most 37 B. |
| Deferred | Compressed executable overlays | Unknown | Requires RAM/lifetime ownership, far-call veneers, cache coherency and an overlay-safe call graph. Consider only after simpler UI sharing and capacity work. |

All estimates in this table are explicitly separate from target-measured deltas. Savings from independent candidates may interact; final integration must be rebuilt.

## Baselines and accounting

The served build used for the initial inventory is 580,984 bytes, leaving 580 of its 581,564-byte application slot. Its main RAM is 89,080/98,304 bytes, low RAM 22,772/29,496, pool 332,876/344,064. The pool requires at least 8,192 bytes spare, so its apparent 11,188-byte gap is not all available.

Inventory (`inventory.json`, `symbols.txt`, `sections.txt`): raw `SMP_DATA` is 303,054 bytes, 52.2% of the served image. Named constant symbols total 339,457 bytes; emitted function symbols total 226,832 bytes. Source attribution groups about 72,448 bytes as UI and 58,852 as DSP/engines, with 95,532 other/mixed. Attribution is approximate: textual function mapping misses/inlines helpers, and the 22,344 bytes marked unmapped must not be described as dead code. Constants/function totals do not account for every alignment gap or load image.

The immutable source snapshot taken during the parent delay-parameter work builds to **582,844 bytes** and exceeds the old slot by **1,280 bytes**. The first baseline measurement used an enlarged private linker region only to obtain a complete linked image; that alone is not a flashable firmware or usable headroom. Baseline files/hashes are in `snapshot-manifest.json` and `snapshot/results/baseline-expanded/`. Further parent changes are outside this frozen comparison.

## Capacity inside the current update boundary

The current physical encrypted app area is **0x4000..0x93000**, 585,728 bytes. The package reserves only 581,564 bytes (`APP_SLOT=0x8DFBC`) for `app.bin`. With the SDK files actually used here:

- App directory/header: 0x120 = 288 bytes.
- `cfg_tool.bin`: 383 bytes.
- Nested config directory and EQ entry: 64 bytes.
- `eq_cfg_hw.bin`: 655 bytes.

Thus the region can hold an application of `585728 - 288 - 383 - 64 - 655 = 584338` bytes. Keeping the app load size word-aligned permits **584,336 bytes (`0x8EA90`)**, 2,772 more than the current slot, with two bytes of trailing slack. This does not claim the next four flash sectors as free space. `FLASH_SIZE`, updater erase range, storage map, update-record scan, SFC plaintext boundary and `_start` can all remain unchanged.

Private evidence is under `capacity/`: maximum-size synthetic application and an actual 582,844-byte frozen baseline image were both packaged; the decrypted JLFS app entry's size and header/data CRCs match, and the whole `flash.bin` remains 0x93000 bytes. Existing loader tests exercise successful installation, no writes outside allowed windows, repeated installation with no extra erases, corrupted transfer retry, irrecoverably damaged package, foreign-key rejection, missing final success and record retention, plus returning from the larger application extent to a smaller old package and the available stock `FM-1_015.fwsc`. OTA staging tests also pass on the maximum-extent package. A capacity-only target build (without preset expansion) fits at 582,844 bytes and its application bytes are SHA-256 identical to the frozen baseline; only the permitted link/package extent differs. Minimal private capacity patch: `capacity/slot-capacity.patch`. These prove the repository loader/package model, not the opaque chip SPL's execution of the new directory layout on hardware.

The integrated packager checks the slot against actual SDK payload lengths before building the image. `tests/package_capacity_test.py` exercises a maximum-size encrypted app, directory/data CRCs, rejection one word beyond capacity, and rejection when SDK configuration grows. The 0x93000 physical boundary and store/loader/crypto ranges are unchanged. Parent loader interruption/corruption and OTA staging tests also pass against the actual integrated package. Hardware gate: boot, updater resume after interruption, update to a smaller MerthSloop package, and stock-return route. Do not change SPL/head flash to obtain this gain.

## Factory preset representation proof

`preset_t` occupies 36 bytes on the target: a four-byte name pointer and 32 remaining bytes (31 semantic field bytes plus one pad). There are **153 records across 12 active tables**, 5,508 bytes. Most optional fields are zero. A naive LZ4 copy of complete records saved only 592 net bytes and is rejected; row-wise zeros and name-pointer overhead are too expensive.

The tested representation uses, for each record, a 32-bit nonzero mask followed only by nonzero field bytes. The full sparse stream is 2,723 bytes and LZ4 is 2,551 bytes. Names are one ROM string stream, with no per-record ROM pointer table. Existing `boot_table_unpack()` expands the sparse stream into the already-existing `proj_tmp` buffer before persistent storage/audio start. A bounded pass restores the record fields and names into 5,508 bytes of `.lowram` arrays. There is no per-note/per-hit decoder, no audio-sample copy, no changed preset content, and no global format or wire change.

### Reproducible build, not linked-address extraction

The earliest size experiment used linked records only as an immutable oracle. The delivered candidate **does not depend on baseline linked addresses or a successful oversized link**:

1. Compile the normal source with `FELUCCA_BOOT_PRESETS=0` to an unlinked target object.
2. Python parses little-endian ELF32 symbol/section/RELA data. It identifies each factory table, validates 36-byte records and expected pointer relocations, resolves name bytes through referenced string symbols, and extracts the other 32 record bytes. It does not execute target code and needs no host C compiler.
3. Generate sparse payload, names, array counts and boot initializer headers.
4. Compile with `FELUCCA_BOOT_PRESETS=1`; the C compiler/linker resolve new name/array pointers normally. Raw source initializer tables remain available for ordinary host tests and the first pass.

Private source/tool patch: `patches/boot-presets.patch`. It guards each engine table, the generated SAMPLE preset table, includes generated counts in core types, and calls `boot_presets_init(&proj_tmp, sizeof proj_tmp)` after existing table restoration and before audio/core 1. Optional SLICE has a guarded declaration as well; the measured build has SLICE disabled. Extra build time is one firmware compile. Upstream merging generally means retaining four wrapper lines around each table; new presets remain ordinary C initializers.

| Frozen target | Image | Main RAM | Low RAM | Pool |
|---|---:|---:|---:|---:|
| Original preset representation | 582,844 | 89,080 | 22,772 | 332,876 |
| Sparse, object-derived, portable boot API | **580,264** | **89,080** | **28,280** | **332,876** |
| Difference | **−2,580** | **0** | **+5,508** | **0** |

The original 581,564-byte slot would now have 1,300 bytes free. Low RAM retains 1,216 bytes; main RAM retains 9,224. This RAM-for-flash trade is explicit and finite; do not keep filling low RAM without checking core-1 stack, interrupt scratch, reverb and linker guards. The preset RAM is persistent after boot, unlike the temporary sparse stream.

Validation: all 153 names and 4,896 field/pad bytes from the unlinked object match the frozen linked oracle. A native oracle checks actual initializer results, repeated initialization and too-small scratch refusal. The actual-restoration 32-bit host suite (matching target record ABI) passes **182 unchanged golden renders**, zero health/voice/routing failures and zero crashes; its harness initializes presets before enumerating their names, matching real boot order. Target checks report no calls in `.ram_text`, unchanged CPU-1 waiting-loop placement and no budget errors. The portable initializer also supports the 64-bit host ABI, where `preset_t` is pointer-size plus 32 bytes. Hardware startup duration and watchdog timing remain unmeasured.

## Structural UI work worth doing next

The useful refactor is a shared representation of a screen's four fields and actions, not blanket noinline annotations or a generic framework for every screen. Current parameter pages, native drum pages, starter pages and held layers separately build labels, values, ranges, knob deltas, percentage bars and dirty hashes. Every new special field can add branches in both input and rendering. The 5,852-byte `layer_screen_draw`, 3,234-byte `draw_columns`, 3,410-byte `step_edit` and separate drum page input/draw functions illustrate where work is concentrated; their whole bodies are not replaceable by one table.

A bounded first slice should cover normal parameter pages plus drum FX remapping and the dynamic delay character fields. Resolve each field once into a small value model: descriptor, value pointer, display override and edit callback/flags. Both the renderer and encoder handler consume that same resolution, instead of independent scope/track/type switches. Keep the audio event paths and note-chain validation untouched. Then convert held modulation views, whose four related modes already share a model, before attempting the step editor's custom gestures.

A **4–10 KiB** total UI reduction is a planning target (~6–14% of the attributed UI code), not a measured promise. Accept the design only after an end-to-end private conversion saves at least 2 KiB net with its new tables included. Hard gates: 79+ framebuffer captures/hash parity where intended; page visibility and knob ownership; held-button layers; full actual-UI fuzz; all action collision/identity tests; native/WASM behavior; target size and render budgets. Fixed small structs, explicit callbacks and one lookup boundary should keep upstream merges localized. Avoid a bytecode interpreter or runtime heap.

## RAM lifetimes and executable compression

Measured symbols include `proj_v3_tmp` 2,584 bytes, `project_undo_saved` 3,816, `proj_tmp` 3,840, `main_scratch` 3,840, `autosave_buf` 3,816 and per-track `undo` 716. These are not all disposable scratch: project undo survives section playback, and autosave/song buffers have simultaneous obligations.

The strongest next lifetime candidate is legacy-project conversion's `proj_v3_tmp`, used only while converting formats 1/2 through format 3. It may fit an explicitly declared member of `main_scratch`, freeing **up to 2,584 bytes of main RAM**. This is an upper bound until all `proj_import` callers prove the source is not aliased with that scratch, storage loads have copied their payload, no reentrant draw/USB callback uses the same union, and all old-format conversion/error tests pass. Track/project undo may be mutually exclusive logically but offers only 716 bytes; first make ownership transitions explicit, and never reuse the project undo snapshot merely because a UI page is hidden.

Freed RAM can support more cold metadata restoration. It does not itself reduce image size. Compressed executable sections could be restored once or switched as UI overlays, but this target's RAM/XIP call-distance constraints, PC/write guards, instruction cache and interrupts make that a separate architecture project. A function may be called from sequencer/MIDI or core 1 even if its source file is named UI. Any overlay must define call-graph ownership and forbid switching out live code; boot/init overlays must finish before IRQs. Do not treat a host compressor's output ratio as net target savings before veneers, decoder code and ownership gates are linked.

## Audio asset checks

`asset-dedup.json` records the direct-read experiment. The 303,054-byte sample blob has 51 zones but only 40 unique starting offsets. Existing GM aliases already avoid **77,064 bytes** relative to independently storing each zone. No remaining complete encoded stream is contained in another. A scan of all stream boundary suffix/prefix overlaps up to 4,096 bytes found only one-byte matches, 37 bytes in total as an optimistic upper bound. Alignment and ordering constraints could reduce even that. No sample deduplication patch is recommended.

Chunk deduplication or stronger compression could shrink the audio blob, but it would add lookup or decode work to the playback path. This study deliberately does not reintroduce the sample packing the user found perceptibly slower, shorten tails, resample sources, or remove factory sounds. A larger build could later offer optional external factory banks, but that consumes user-storage capacity and changes deployment rather than being free headroom.

## Physical flash map and partition-expansion gate

All ranges below are half-open and come from the current loader/storage/HAL definitions; the SDK directory's VM/USR descriptors must also be reconciled before changing layout.

| Flash offsets | Current role |
|---|---|
| 0x00000..0x04000 | SPL, flash/JLFS head and chip-key config; updater deliberately never writes it |
| 0x04000..0x93000 | Encrypted app directory, application and SDK configuration |
| 0x93000..0x97000 | Apparent four-sector gap; inside SPL/update-record scan region and SDK VM allocation |
| 0x97000..0x9F000 | Four projects, A/B sectors |
| 0x9F000..0xA0000 | Autosave A |
| 0xA0000..0xDC000 | User sample slots |
| 0xDC000..0xE0000 | Two A/B user preset banks |
| 0xE0000..0xE5000 | OTA loader staging; valid update record at 0xE4F00 |
| 0xE5000..0xE7000 | FM6 bank A/B |
| 0xE7000..0xFB000 | Fourth user sample slot |
| 0xFB000..0xFC000 | Unallocated sector, still in update-record scan domain |
| 0xFC000..0xFE000 | Settings A/B |
| 0xFE000..0xFF000 | Autosave B |
| 0xFF000..0x100000 | SDK key/MAC sector; preserve |

The tempting 16 KiB at 0x93000 is **not validated free app capacity**. `ldr_records_drop()` scans every 4 KiB boundary−256 from 0x93000 to 0xFC000 and erases a whole sector if its bytes resemble a valid update record. Placing arbitrary code there while retaining that scan could delete code after a successful update. The runtime SFC plain window also starts at 0x93000, so encrypted application bytes there would be read incorrectly. The package VM descriptor starts there, and older loaders only write below it. Merely extending `app.ld` changes none of these contracts.

Before a genuine expansion is proposed: map the immutable SPL's exact record scan and app-file handling; constrain record creation/cleanup without losing vendor recovery compatibility; define package-version migration and old-loader behavior; move/configure the plain window consistently; update JLFS VM/PRCT sizes and package CRCs; test power loss at every erase/program/commit transition; preserve stock return and Flipper recovery handoff; prove factory boot and settings/sample access; then test hardware. Never gain space by discarding persistent data or overwriting SPL without explicit scope. No live flash, partition migration, or SPL change was performed.

## Recommended sequence

1. Validate the integrated extra 2,772 bytes on hardware inside the current physical app boundary. Package and host loader checks pass; this is the lowest-runtime-cost candidate.
2. Validate cold startup of the integrated 2,580-byte preset representation on hardware. Its two-pass generator and actual-restoration checks are now part of the build/tests; retain its explicit RAM-for-flash accounting.
3. Freeze another source baseline and implement one shared UI field-resolution slice. Require measured multi-KiB savings before expanding the refactor.
4. Prove conversion scratch lifetime sharing to replenish main RAM, then assess further cold metadata restoration.
5. Treat true flash repartitioning and executable overlays as separate designs with recovery gates, not routine linker tweaks.
