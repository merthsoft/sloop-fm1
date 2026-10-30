# Flash .99 startup-data audit

The recommended literal-order patch is now integrated. [Combined .100 build and new presets](CONSOLE-SOUNDS-EXPANSION.md) records actual final capacity and validation; independent savings below retain the frozen .99 baseline and are not additive promises.

The recommended two-line generator change saves **108 linked flash bytes without additional RAM** on a frozen copy of the current dirty .99 source: **583,728 → 583,620 B**, leaving **716 B flash**. It changes only startup literal placement order. It does not remove instruments, alter IDs, compress playback samples, change a runtime decoder, or expand any RAM destination.

Production source, shared build outputs, version, installer and Git state were not changed. The new report is the sole non-private file. Parent is adding presets concurrently; these numbers are deliberately measured on the frozen .99 snapshot and must be remeasured after integration rather than added to unrelated deltas.

## Baseline and measurements

Read before experiments: FLASH-91-DATA-AUDIT, LITERAL-STORAGE, UI-REUSE-INTEGRATION and the relevant existing startup packers. Drum dictionary similarity ordering, extended grammar control bytes, bounded RAM literal destinations, minimum-cost LZ4, packed preset fields/names/zones and subdivision pointer sharing are already baseline. Their historical savings are not counted here.

Snapshot and evidence: `build/flash-99/data/`. `source/` copies the current dirty firmware/tools/tests and generated headers; it is not Git HEAD. Builds use the pinned JieLi Linux 20250324.1 toolchain, unchanged -Oz, verified SDK inputs and FELUCCA_AUDIO_DIAG=0. `baseline.log` reproduces the parent's exact 583,728-byte image. All builds retain 97,612 B main static RAM, 28,928 B low RAM and 335,424 B pool, with the existing 8,192 B pool reserve.

| Frozen configuration | Image bytes | Saved bytes | Flash free bytes |
|---|---:|---:|---:|
| Dirty .99 baseline | 583,728 | 0 | 608 |
| Lexical assignment only | 583,632 | 96 | 704 |
| **Lexical assignment + reverse lexical layout** | **583,620** | **108** | **716** |

These are complete normal target links. The recommended combined result is measured directly, not the sum of standalone estimates. `baseline-build/`, `lex-build/` and `source/build/` retain the actual ELF, object, disassembly, generated headers and image. `measured-summary.json` records hashes, payload totals and restore-function sizes. `recommended.patch` changes only `tools/pool_literals.py`; the two incremental patches are retained separately.

Baseline image SHA256: `9a32801a341ecf2e5d8d4ba4542a14ce008534415756ea046b3bc79b262df28b`.
Recommended image SHA256: `2b6614c09a691ebb6067c65511413fd2a07c106e076e95d7f8deecef48a7a4e7`.

## Why it saves

Root discovery remains longest-first, preserving the existing complete-object substring elimination. Only after roots are discovered does assignment iterate them lexically, rather than in discovery/length order. Both destinations still contain exactly 5,500 and 2,500 restored bytes. Their membership changes: lexically related labels tend to share a destination, which improves the existing shared grammar and LZ4 compression. Each destination's roots are then laid out reverse-lexically. Symbol offsets and final pointer relocation are recomputed by the existing verified machinery; no lookup or indirection is added.

Baseline packed main/pool streams are 3,112 / 1,647 B; recommended streams are 3,244 / 1,408 B. The shared dictionary stays 318 B and the flash literal remainder stays 1,782 B. Total stored payload falls **107 B**, becoming **108 B linked** after alignment. Original literal objects and symbol counts remain identical. The report's concatenated-stream hash changes because grouping/order changes; exact original-object comparisons, rather than that layout hash, are the behavior oracle.

No target C source changes. Runtime code, call structure, bounded eight-byte grammar expansion stack, RAM destination limits, boot ordering, ownership and failure behavior remain unchanged. The existing `boot_table_unpack` remains 636 target bytes. Token distribution and LZ4 match shape differ, so physical startup duration remains unmeasured; no startup speedup or hardware stack high-water claim is made. There is no per-note, frame or audio playback cost and no resident cache. After startup, all strings are still direct pointers to ordinary restored bytes.

## Exact checks

`unit.log` and `reverse-unit.log`: four original object-format/determinism/embedded-zero/refusal tests pass.

`target-literals.log` and `reverse-target-literals.log`: actual candidate target transformation is deterministic; **1,411 complete original objects**, all final relocated addresses, three real cold restores, destination guards and UBSan pass. This uses the pre-transform target object as the independent original-byte reference, not newly generated expected strings alone.

Every target link passes the normal RAM-region, RAM-code/no-call, second-core loop placement, persistent scratch and HAL register-access checks. `sample-identity.json` verifies the entire unchanged 303,054-byte SMP_DATA: SHA256 `dcad9fe8f8c08a796d1b433c5cc0f96af46416502833b256593994fe7cccfe17`.

Full UI/audio suites were not rerun by this specialist: source/runtime data equality and target relocation proof are strong evidence, but the parent should run its normal integration validation against the new preset bank. No device was flashed.

## Rejected and unmeasured ideas

Private `sweep.json` examines fewer grammar codes, 80–156 in steps of four: none beats the existing 159-code baseline on the original assignment. Merely reducing dictionary count is not a saving; it increases stream size more.

`orders.json`, `assignments.json`, and `lex-orders.json` retain deterministic alternative sorting/assignment experiments. Suffix, length, reverse, compressed-ratio and incompressible-first assignments generally worsen payload. Pure reverse layout without lexical assignment saves only 3 payload bytes; it is not claimed as a standalone target win. Selected lexical assignment and reverse layout are two simple deterministic sorts rather than a data-specific permutation table or expensive stochastic optimizer.

An extra LZ4 layer over existing six-bit preset names saves only 26 payload bytes before extra boot plumbing, so it was not implemented or target-linked. Raw ASCII-name LZ4 is larger than the existing six-bit format. New five-bit name grammars, per-table field permutations and descriptors were not implemented; their extra decoding/indexing cost is unmeasured. Existing preset IDs, sound content, table semantics and all samples remain untouched. No RAM-budget increase or literal-cache trade is proposed.
