# Flash .91 constant-data audit

The recommended pair saves **216 linked flash bytes without adding RAM**: normal .91 goes from **584,296 to 584,080 B**, leaving **256 B** in the unchanged 584,336-byte application slot. An optional 256-byte main-RAM trade reaches **584,004 B**, leaving **332 B flash** and only **548 B main static RAM**. These are complete target links, not summed payload estimates. This audit did not establish a multi-kilobyte data-only win within the remaining RAM budget.

Only this new report is outside the private evidence directory `build/flash-audit-91/data/`. Production firmware, assets, installer, version, existing reports and Git state were not changed. No flashing, publishing, commits or delegation occurred.

## Baseline and evidence

The immutable input is `build/flash-audit-91/baseline/source`, including the uncommitted .91 hold-delay/worker hardening changes. Git HEAD was never substituted. A copied source reproduced **584,296 B byte-for-byte**, SHA-256 `c98a63741a9f16f2b9f7faa753dd31c980241a7013e56121339fbb03274552ad`. Its main/low/pool extents reproduce **97,500 / 28,928 / 335,424 B**. Capacities are 98,304 / 29,392 / 344,064 B; the 8,192-byte pool reserve leaves only **448 B beyond reserve**. Older reports' 29,496-byte low-RAM capacity does not apply to this frozen image.

Read before experiments: ARCHITECTURAL-HEADROOM, ENGINE-PERFORMANCE-INTEGRATION, HEADROOM-COMPARISON, LITERAL-STORAGE and OPTIMIZATION. Their preset/zone/font/icon/table packing, suffix-pooled literals, grammar, minimum-cost LZ4 parsing, metadata restoration and exact engine control tables are already implemented. Their historical savings are not new candidates here. Runtime sample packing, physical expansion, and the rejected cold-state initialization are not revived.

All experiments use the pinned JieLi Linux toolchain `jieli-linux-toolchains-20250324.1`, the pinned three SDK payloads, existing -Oz and **FELUCCA_AUDIO_DIAG=0**. Builds ran sequentially in WSL Ubuntu. The frozen build helper skips generation and preserves the source version; generated-header adjustments are therefore explicitly retained as private evidence. No elapsed build or shared-host timing is presented as a performance result.

Evidence paths below are relative to `build/flash-audit-91/data/`:

- `baseline-reproduced.log`, `reproduced-build/`: byte-identical reproduction, ELF/disassembly/package.
- `rows-build/`, `ram-build/`, `controls-build/`, `combined-build/`, `recommended-build/`, `triple-build/`: actual independent/combined artifacts; corresponding `*-build.log` files contain all region and placement checks.
- `measured-summary.json`, `summarize.py`: ELF-derived RAM extents, image lengths, function sizes and sample hashes. `constant-inventory.json`, `duplicates.json`: linked constant census.
- `01-drum-row-order.patch`, `02-control-grammar.patch`, `03-optional-main-literals.patch`: source patches in application order.
- `prepare_rows.py`, `prepare_evidence.py`, `build_matrix.py`, `row-orders.json`, `sweep.json`: transformations, independent oracles and search evidence. `source/` is the final private three-candidate source.

## Ranked measured candidates

Every row below is independently linked against the same frozen baseline unless marked combined. Low RAM remains 28,928 B and pool remains 335,424 B in **all** rows. The pool reserve is never spent.

| Rank / configuration | Image B | Net flash saved B | Flash free B | Main static B | Main free B | Additional persistent ownership |
|---|---:|---:|---:|---:|---:|---|
| Frozen reproduced baseline | 584,296 | 0 | 40 | 97,500 | 804 | None |
| 1. Similar synthesized-drum dictionary rows adjacent | 584,152 | 144 | 184 | 97,500 | 804 | None |
| 2. Use unused control bytes as literal-grammar tokens | 584,224 | 72 | 112 | 97,500 | 804 | None |
| 3. Optional main literal limit 5,500 to 5,756 B | 584,228 | 68 | 108 | 97,756 | 548 | 256 B main BSS |
| 1 + 3, measured intermediate | 584,084 | 212 | 252 | 97,756 | 548 | 256 B main BSS |
| **1 + 2, recommended combination** | **584,080** | **216** | **256** | **97,500** | **804** | **None** |
| 1 + 2 + 3, optional combined maximum tested | 584,004 | 292 | 332 | 97,756 | 548 | 256 B main BSS |

The optional RAM increment adds **76 B** flash saving to the recommended pair, versus its independent **68 B**. This interaction is why standalone deltas must not replace the combined link. I recommend preserving the 256 B main margin unless a specific feature needs the extra flash.

## 1. Order immutable drum dictionary rows for compression

The largest non-audio packed table is BOOT_DS_BASE: **2,617 B** representing **242 distinct 17-byte rows**, 4,114 restored bytes. Discovery order follows kit construction rather than similarity. Its runtime representation already uses a compact row dictionary, so duplicate rows have already been eliminated.

The generator now packs each base into its existing byte representation, starts with row zero, then deterministically chooses the unused row with the most equal bytes to the previous row. Equal scores choose the lowest original index. It remaps **every** DS_KITS base index through the resulting permutation. Names, styles, crush values, gains and lane order remain unchanged. There is no extra runtime index table or decoder.

Minimum-cost LZ4 payload results: original order 2,617 B; lexical 2,494; reverse lexical 2,490; common-prefix nearest 2,502; byte-equality nearest **2,462**; matching-four-byte-window nearest 2,460. The simple byte-equality rule was selected: the more complicated window rule saves only two payload bytes. The chosen **155-byte payload reduction becomes 144 linked bytes**, including ordinary alignment/string-object/layout effects. No dictionary/relocation cost is omitted from the actual image measurement.

Runtime ownership and cost: the same 4,114-byte DS_BASE destination is restored at the same existing boot phase, and later becomes immutable shared data. The same kit base-index load and the same 122-byte `ds_factory_sound` function remain. Playback still sees ordinary direct table bytes; the sample access path is unchanged. Boot uses the existing bounded LZ4 decoder. Row reordering changes its match pattern; target startup duration remains unmeasured. No new C stack object or persistent allocation exists.

Proof: `rows-oracle.log` verifies 242 exact row permutations, all 32 kit names/styles/crush fields, all 512 original sound row selections and gains. The independent original oracle `factory-sounds-oracle.bin` contains **11,264 original sound-field bytes**. `factory-sounds-test.log` runs the actual production `ds_factory_sound` function after real boot-table restoration under UBSan, checking every byte and modulo kit/lane behavior. This is stronger than comparing a newly generated candidate oracle only. `rows-tables.log` checks all **22,032** restored table bytes, FIR data and malformed decoder bounds.

## 2. Extend the existing literal grammar with unused control-byte tokens

The existing shared grammar already uses unused values 128..255. The candidate also offers **1..31**, after those high values. Values present anywhere in the original literal streams are never assigned. NUL remains a literal. The dictionary index is `v-128` for high tokens and `v+127` for offered low tokens; other bytes are not dictionary codes. Explicit zero holes preserve occupied original values. Children still reference earlier-created symbols and retain the existing depth cap for an eight-byte expansion stack.

Baseline payload: main/pool **3,197 / 1,705 B**, dictionary **256 B**. Independent candidate: **3,111 / 1,643 B**, dictionary **318 B**. The 148 B stream reduction minus the 62 B larger dictionary gives **86 B payload savings**. The real restore helper grows **182 to 206 target bytes**. The complete linked image saves **72 B** after all metadata/layout/relocation effects; neither payload arithmetic nor helper-size arithmetic alone is a net-image measurement.

RAM and runtime: restored destinations stay exactly **5,500 / 2,500 B**, at their existing linker-owned NOLOAD BSS/pool locations. Startup performs additional token-range/index work, with fewer input tokens: baseline 3,379/1,711 versus candidate 3,264/1,645. This is not a claim of faster boot. Both target helper prologues save `{rets,r10-r4}` and allocate 24 local bytes: **56 B of own frame including saved registers**, unchanged. The bounded eight-byte expansion stack remains; called-decoder/caller frames and hardware stack high-water are not established by that local frame count. No per-note/frame/audio decoder is introduced.

Proof: Python complete-object/embedded-NUL tests, deterministic generation, original 0..255 byte round trips, and malformed ELF/relocation refusal pass (`controls-unit.log`). `controls-grammar.log` adds actual C tests for codes 1 and 31, nested high/low expansion, code-2 cycles, preserved NUL/space/DEL/255, output guards and malformed sizes under UBSan. Existing high-token/nested/overlap tests remain. The target-object oracle verifies all complete original literals, final linked addresses, three actual cold restores and guard bytes under UBSan (`controls-target.log`, `recommended-literals.log`, `triple-literals.log`). Build reporting counts 1,404 selected string symbols; the target check covers 1,405 complete mergeable objects across the original object sections.

Early-boot contract is unchanged: main startup zeroes the relevant BSS/pool ranges, restores ordinary tables, restores literals, then restores sample zones and presets, before UI/storage initialization, interrupts/audio operation or core-1 launch. Restore failure still enters ROM recovery. The transform retains ELF-symbol relocation through the normal linker, never serialized linked addresses. No shared mutable scratch or allocator ownership is introduced.

## 3. Optional 256-byte main literal trade

Only `pool_object`'s default main limit changes, 5,500 to 5,756. The pool limit remains 2,500. Startup code is unchanged; selected objects and final addresses are rechecked. This permanently owns another 256 B of ordinary BSS and leaves **548 B** static main margin; it does not borrow scratch or assume another audit's RAM savings.

Independent payloads become 3,385/1,707 B, flash literals 1,496 B, dictionary 256 B: only **66 B payload reduction**, and **68 B linked savings**. This is a poor RAM exchange compared with the first two candidates. The triple build is nonetheless measured for a concrete integration option. Its original bytes/final addresses and repeat restoration pass. Startup expands 256 more destination bytes; stack structure remains unchanged.

Unlinked sweeps also tried +256 pool bytes, +256 in both regions and +512 main/+256 pool. Their gross stored-data savings versus baseline are 65, 159 and 213 B respectively. They are **not target-linked candidates or net savings claims**. Spending pool's scarce 448 B above reserve for this return is not recommended. No such allocation was retained.

## Additional inventory, rejected searches and larger unmeasured ideas

The linked sample blob is **303,054 B**, SHA-256 `dcad9fe8f8c08a796d1b433c5cc0f96af46416502833b256593994fe7cccfe17`, byte-identical in **every** independent and combined image. No sample bytes, quality, zone selection or playback access changed. Prior complete-stream deduplication is already shipped; its historical result is not counted again.

Remaining larger constants include DS_KITS 1,408 B, TP 1,136 B, GP 944 B, PAGES 624 B, twelve 184-byte engine descriptors, compressed preset fields 2,517 B, preset names 1,161 B, compressed font pixels 1,546 B and compressed zones 794 B. Many contain pointers or already-compressed data. A byte-identical complete-object scan of linked read-only objects at least eight bytes finds only **N_ONOFF/N_ONOFF_E, eight bytes**. This excludes inter-object substrings, relocation-normalized equivalence and smaller objects, so it is not a universal redundancy proof. It does show that simple whole-object deduplication cannot supply a multi-KiB gain here. Address identity would need review even for that eight-byte alias; no alias patch is proposed.

Grammar limits 0,32,64,80,96,112,120,128 were measured at build-payload level. Stored literal cost monotonically improves from 8,022 to **6,910 B** in those trials. Reducing the current dictionary is rejected by this evidence. The extended grammar candidate uses genuinely unused code values rather than discarding original byte values.

Potential larger work, **unimplemented and unmeasured**:

- Relocation-aware compact descriptors: TP/GP/engine metadata use several full pointers per 16-byte descriptor. Relative 16-bit offsets or shared descriptor IDs might reduce flash, but need source-owned generation, explicit range checking, preserved null semantics and a measured access/formatting implementation. Decoder/accessor code, hot descriptor calls, relocated string destinations and upstream maintenance can consume the saving. No KiB estimate is claimed.
- Joint startup metadata compression or row/column transforms across factory-preset fields, font pixels or FM6 factory records could improve correlation. Existing exact table algorithms already compress these heavily. Any transpose must prove in-place expansion or explicit scratch lifetime without allocating an unbudgeted full-size buffer. Cross-block LZ4 dictionaries need real decoder support and destination/overlap proof. A host compressor ratio is not a linked result.
- Restoring all DS_KITS (1,408 B) or all twelve engine records (2,208 B) requires more contiguous persistent RAM than any currently free region provides. Aggregate free bytes across main/low/pool are not one buffer. These ideas are deferred until independently proven RAM recovery exists, with no assumed gain from another audit.

## Reproduction and application order

For ordinary source integration, apply `01-drum-row-order.patch`, then `02-control-grammar.patch`. Apply `03-optional-main-literals.patch` only if the additional permanent main ownership is accepted. The first two are independent source transformations; the third changes the same pool generator's default, so apply it last. The grammar encoder and C decoder must always land together. Retain the accompanying C grammar test changes.

A normal full generation produces raw drum headers and then the existing boot-table packer. Do not transplant a baseline compressed header over the regenerated candidate. For this frozen no-generation audit, `prepare_rows.py` reconstructs the packed DS_BASE header, remapped kit indices, boot descriptor length and expected boot bytes from the retained raw generated candidate, while proving original selection semantics first. Source patches are the integration artifact; private generated headers show exactly what was linked. `prepare_evidence.py` emits the source-only patches and the independent sound oracle. The private literal-test path adjustment merely points to the existing host compiler wrapper and is not an integration patch.

Reproduce from each private source using WSL Ubuntu, environment `FELUCCA_AUDIO_DIAG=0` and `JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, then `python3 build/build-target.py`. Host C checks use the existing `build/host/optimization-cc` wrapper with `-UNDEBUG -O2 -fwrapv -fno-strict-aliasing`, UBSan and nonrecovering failures. The retained Python scripts and logs give exact file paths and arguments. Do not run target builds concurrently in this directory.

## Remaining qualification

All target links pass region limits, RAM-code/no-call placement, core-1 waiting-loop placement and register-access checks. Real literal and table restoration, malformed-input boundaries, original factory metadata and sample-byte identity pass. The inherited boot-table test's font fixture emits two excess-initializer warnings for newer fixed metrics; it does not use that struct to compare the restored table/FIR bytes. The inherited note-chain missing-braces warning also remains.

A full audio-golden/UI suite was **not** rerun in this specialist audit. No synthesis or UI code changes except the startup-only grammar decoder are proposed; exhaustive original metadata equality and unchanged factory sample bytes support behavior preservation, but normal final integration should still run the repository's complete suites. Hardware remains required for cold/warm/recovery boot, watchdog duration, real startup stack high-water and audio deadline/sound checks. No device timing, stack margin or audible improvement is claimed from host tests. Version, package formats, storage recovery, safety guards, admission/overload policy and the retained pool reserve are unchanged.
