# Cold persistence and editor/control headroom audit

2026-10-23. Analysis only; production firmware and shared generated/build outputs were not changed. All experiments, patches, logs and binaries are private to `build/headroom-storage-51/`. No package, Git ref, installer or device was updated.

The useful result is **hundreds of flash bytes plus 2,584 bytes of main RAM**, not a multi-KiB flash reduction. The complete prototype saves **416 image bytes** and **2,584 main static RAM bytes**. A smaller persistence-focused combination saves **256 image bytes** with the same RAM recovery. Legacy formats and existing protocol IDs remain supported.

## Frozen baseline and controlled measurements

Read the architectural headroom, redundancy, comparison and optimization reports and reviewed the earlier private headroom experiments before making candidates. In particular, preset restoration, icon cleanup, shared entry transactions, starter metadata reset and existing scratch sharing are already integrated; their historical savings are not counted again.

`source/` was copied from the **working files**, including the uncommitted obsolete-icon removal and compact header atlas, rather than exported from HEAD. `manifest.json` records initial firmware/tool/test/generated-file hashes. `original/` and `original-tests/` retain files changed during experiments. The reproduced baseline is exactly **581,296 bytes**, matching the requested source checkpoint.

All application comparisons use the same source/version, generated assets, pinned JieLi 20250324.1 toolchain, `-Oz`, `FELUCCA_SLICE=0`, two-pass preset generator and linker layout. `run.py` calls `build_app()` and `check()` within the private snapshot. It does **not** call the package/loader build or consume shared build outputs. SDK packaging was outside these experiments.

| Private candidate | Application B | Flash saved B | Main RAM B | Low RAM B | Pool B |
|---|---:|---:|---:|---:|---:|
| Baseline | 581,296 | — | 89,128 | 28,280 | 332,876 |
| Direct legacy conversion | 581,248 | 48 | 86,544 | 28,280 | 332,876 |
| Persistence combination: direct + validation + bank + select + initall | **581,040** | **256** | **86,544** | **28,280** | **332,876** |
| Complete combination: above + notify + settings | **580,880** | **416** | **86,544** | **28,280** | **332,876** |
| Typed conversion-scratch sharing, alternative to direct conversion | 581,296 | 0 | 86,544 | 28,280 | 332,876 |

The complete candidate leaves **3,456/584,336 flash bytes**, **11,760/98,304 main RAM bytes**, and the same **1,216/29,496 low RAM bytes** free. The pool gap remains 11,188 bytes with its existing 8,192-byte reserve. These are static linker margins, not stack high-water or deadline measurements.

For the complete candidate, `.text` is `0x8d0dc → 0x8cf3c` (−416); `.bss` is `0x15a18 → 0x15000` (−2,584). `.data`, `.ram_text`, `.c1_text`, `.lowram`, `.pool` and `.noinit` sizes are unchanged. The persistent `.scratch` subrange falls from 8,800 to 6,216 bytes. Complete section/symbol listings and application hashes are in each `source/results/<mode>/` and `measurements.json`.

## Ranked candidates

### 1. Convert formats 1/2 directly through the shared old-track mapper

Source: `firmware/src/project.c:212`, `:233`, `:332`, `:334`, `:352`, `:380`. Patch: `direct.patch`.

Formats 1/2 currently create an entire 2,584-byte format-3 project solely to feed the existing format-3 conversion. Their tracks differ from format 3 in parameter count, while engine, preset and eight-byte-step order are otherwise identical. One count-aware old-track mapper can read either validated record directly, retaining missing-parameter defaults, swing scaling, synth steps, GM drum-lane conversion and engine/preset handling. Format 1 still initializes the other three tracks to their existing defaults.

The patch removes the intermediate transaction and buffer, rather than removing any converter. Compile-time offset assertions pin the two historical layouts. No extra stack array, heap allocation, persistent format change or audio-loop helper is introduced. Current production callers keep source and destination separate: section fetch and autosave use `proj_tmp`; editor restore stages in `proj_tmp` and imports into `autosave_buf`. This should remain an explicit caller invariant; the refactor does not establish general in-place legacy conversion support.

Measured alone: **48 flash bytes and 2,584 main RAM bytes saved**. This is preferable to the alternate `scratch.patch`: typed scratch sharing recovers the same RAM but saves no flash and moves legacy type declarations into `scratch.h`. Do not combine their RAM savings.

### 2. Share the settings snapshot/commit/cache transaction

Source: `project.c:1010`, `editor.c:508–568`, `editor_dsyn.c:25`. Patch: `settings.patch`. **80 flash bytes saved alone; RAM unchanged.**

SYN/MIDI/theme backup restores and SYN STORE repeat `persist_fill`, `settings_write`, successful `persist_saved` update and status conversion. A cold `settings_commit_current` helper shares that tail. Request-specific length/magic/range validation, playing/transport refusal, staging-buffer ownership and mutations stay in their original callers. Failed writes still leave `persist_saved` unchanged. Existing settings composition, optional MIDI trailer and side record are untouched.

Target build/placement/budget checks pass. The host suites below do not execute the complete editor/settings restore state machine; integration should add actual BK_PUT/DSYN_STORE failure/retry and reply tests before taking this optional 80-byte change.

### 3. Share editor step-change shadow invalidation

Source: `editor.c:1049`, `:1119`, `:1159`, `:1182`. Patch: `notify.patch`. **80 flash bytes saved alone; RAM unchanged.**

DRUM_STEP, LOCK_SET, MICRO_SET and FILL_SET repeat redraw invalidation plus selected-track signature refresh to suppress echo pushes. `ed_step_edited(track, step)` shares only that tail, after each original IRQ-protected mutation. Bounds, lock-capacity errors, drum versus synth handling, reply serialization and IRQ boundaries remain in their callers. Getter paths are not changed.

This is control bookkeeping, not rendering or sequencer/audio processing. Like settings sharing, it needs actual editor WATCH/reply tests across selected and non-selected tracks before integration; the size and target checks alone do not prove wire-state behavior.

### 4. Share historical project envelope validation

Source: `project.c:246`, `:274`, `:322`, `:334`, `:352`. Patch: `validation.patch`. **64 flash bytes saved alone; RAM unchanged.**

Five converters separately check exact input length, magic, stored size and trailing FNV hash. `proj_record_ok` performs the same checks in the same short-circuit order and uses `memcpy` for word access. It never reads the header or trailer until the exact expected length matches. Each format retains its own payload conversion. No checksum, bounds or semantic check is weakened.

`both.patch` combines this with the preset-bank predicate below: **96 bytes saved** in a separately linked comparison.

### 5. Share A/B winner ordering between copied and XIP loads

Source: `storage.c:95`, `:122`. Patch: `select.patch`. **48 flash bytes saved alone; RAM unchanged.**

`st_current` and `st_xip` independently read both headers, choose the highest unsigned sequence (A on ties), validate its body and retry the other copy after failure. A shared `st_select` keeps those policies together. RAM reads still use `st_read`/`st_body`; XIP reads still hash mapped bytes without copying. The candidate does not change sequence-wrap behavior, on-flash addresses, commit order, CRCs, write readback or side-record ownership.

Existing torn-write/XIP tests and the independent 2,160-case matrix below pass. This changes a durability primitive for only 48 bytes; review its error paths deliberately even though its output selection is exhaustively checked over the stated matrix.

### 6. Reuse existing preset-bank validation; share project initialization

Source: `upreset.c:77` (`up_bank_ok`), `editor.c:491`; format converters in `project.c`. Patches: `bank.patch`, `init.patch`.

Backup restore duplicates the exact bank length/magic/record-size/slot-count predicate already used at boot. Calling `up_bank_ok` saves **32 bytes**. The caller retains its special zero-length empty-bank case and playing/flash checks.

Sharing the zero/header initialization of the three original converters saves **32 bytes** alone. After direct conversion there are five such callers; `initall` shares those in the measured combinations. There is no standalone claim for the five-caller variant. All initialization clears and current-format header values are retained.

## Rejected opportunities and required differences

* **Sector-address table:** `sector.patch` replaces the arithmetic/branches in `storage.c:55` with an 18-byte sector-number table and bounds guards. Final image remains **581,296 bytes**. Reject: no linked saving and another persistent-map representation to maintain. Guards matter because `st_xip` must tolerate an invalid object without indexing outside the table.
* **Replace three staging-copy loops with guarded memcpy calls:** `copies.patch` also remains **581,296 bytes**. Reject for flash headroom. The private implementation guards zero-length null-source calls; unguarded libc calls would add a separate portability concern. No claim of better target timing is made.
* **Remove repeated settings loads at boot:** `persist_boot` loads the settings prefix, `st_side_load` selects its committed side record, and `dsu_boot` later loads the settings/SYN/MIDI payload. These are real repeated reads, but intervening `proj_fetch` operations overwrite `st_buf`. Simply deleting the last read consumes unrelated project bytes. A single boot-reader transaction would require reordering ownership and carrying the selected header; no measured saving is claimed.
* **Merge explicit section and autosave transactions:** `project_save.c:10–82` writes a section and separately refreshes the startup object. These have different object IDs and recovery obligations. `sections_write` intentionally captures the working loop after queued snapshots, including later edits; `persist_flush_now` and autosave have their own timing/hash policies. Collapsing them would alter what resumes after reset. Existing retries and readback remain intact.
* **Delete historical project/protocol code:** formats 1–5 are reachable from `proj_fetch`, autosave and BK_PUT. Old STEP commands deliberately translate GM notes to/from drum lanes, whereas DRUM_STEP carries full lane/level/ratchet state. Selected-track commands and TRACK_* have different addressing and WATCH behavior. These are required compatibility, not dead copies.
* **Remove retired-command replies:** `editor.c:685` explicitly answers unavailable for 88/89. Android's `design docs/FIRMWARE.md` and `STATUS.md` rely on that behavior. SYN 72–76, LIVE 77, performance 86 and grooves 87 remain active. A small unavailable reply is not retained implementation of the retired feature. Source-only disabled developer alternatives are not counted as linked savings.
* **Unify panel validators blindly:** `panel_init` checks safe ranges/directions; backup `panel_valid` additionally requires permutations. Sharing by dropping the latter weakens restore validation; applying it everywhere changes malformed warm-state recovery. No equivalent measured candidate is offered.
* **Generic command-dispatch/serialization interpreter:** the largest emitted control function is `ed_handle` (4,542 bytes); much of it is distinct schemas, replies, stop/IRQ/ownership rules. The audit's prefix-based cold-symbol inventory totals about 26,546 bytes, but is not a call-graph attribution or removable-code estimate. Existing pack7, v14, parameter, bank and A/B primitives already share much of the obvious serialization. No multi-KiB reduction is substantiated here.

## Validation and evidence

The baseline, sector, bank, init, typed-scratch and validation+bank experiments each passed the existing project, storage, user-preset, upstream-project, section-save, delay-storage and chord-voicing-storage host suites. The **complete final candidate** passes all seven again; logs and `validation.json` are in its result directory. Tests retain assertions under optimization and use pinned Zig 0.13. The direct-conversion patch changes only two obsolete scratch-poison setup lines in `project_test.c`; all original conversion assertions remain.

The final independent import oracle generates 1,000 records per format, checks successful output and six malformed envelope cases: **6,000 accepted conversions plus 36,000 rejections** per build. It compares every byte of the 6,000 output records, not merely musical fields. Baseline, direct and complete candidate produce identical **22,896,000-byte** output blobs and stable digest `8b386c47`. `equivalence/` contains binaries, compile logs, output blobs, result logs and the final comparison JSON. The first oracle used a declared word array and produced code-shape-dependent hashes; it was discarded. Final input storage is allocated, initialized through character stores, and emitted for direct byte comparison; repeated baseline/direct executions agree.

`storage_audit_selection.c` checks **2,160 cases** across every object, independently varying valid headers, valid payload CRCs, both sequence orderings, ties, 0/UINT_MAX extremes and lengths 0/1/600. Copied loads and XIP choose the expected copy and bytes; invalid object IDs are rejected. Existing suites additionally check torn data/header/side writes, empty objects, multipart settings, bad bank shapes, newest-copy corruption and fallback.

All measured successful target candidates pass placement/RAM guards, static DSP budgets and all **24 hot-function loop-call caps**. `.ram_text` remains 925 instructions with no calls; the core-1 waiting loop remains 60 bytes. Final MMIO checks return no errors. No audio-render or UI-render source was modified. This audit did not rerun audio goldens, perform end-to-end phone/web wire transactions, measure hardware boot/watchdog timing or establish physical audio deadline margin.

## Reproduction

From WSL Ubuntu, change directory to `/mnt/c/code/sloop-fm1/build/headroom-storage-51`. The checked-in-to-audit scripts use only private source/output paths. `run.py` pins `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`; host verification pins `/home/shaun/.cache/sloop-host-tools/zig-linux-x86_64-0.13.0/zig`.

```sh
python3 quick.py baseline direct direct+validation+bank+select+initall direct+validation+bank+select+notify+settings+initall
python3 equivalence.py baseline direct direct+validation+bank+select+notify+settings+initall
python3 verify_final.py
cd source
/home/shaun/.cache/sloop-host-tools/zig-linux-x86_64-0.13.0/zig cc -UNDEBUG -O2 tests/storage_audit_selection.c -o ../selection-test
../selection-test
```

`candidates.py <mode>` always restores frozen originals before applying a mode. Individual modes are `validation`, `bank`, `init`, `scratch`, `sector`, `copies`, `select`, `notify`, `settings`, `direct`, and `both` (validation+bank); combinations use `+`. Each emits an applicable unified patch. Apply **one combined patch**, not its overlapping component patches. Line-ending normalization in `equivalence.py` is for its disposable copies. Failed early prototype/parser/oracle attempts are superseded by the successful final logs and patches.

## Integration recommendation

Start with `direct+validation+bank+select+initall.patch`: **256 flash bytes and 2,584 main RAM bytes recovered**, with the legacy-output and durability evidence above. Review A/B selection carefully or omit that 48-byte component if durability churn is not worthwhile. Add the optional settings/notification helpers only after actual editor transaction and WATCH tests; their complete linked result is **416 bytes saved**, not a sum to add to another specialist's estimate. Rebuild and revalidate the final integrated source. This scope offers useful RAM recovery and modest flash savings; it does not justify deleting compatibility, changing partitions or reintroducing runtime sample decompression.
