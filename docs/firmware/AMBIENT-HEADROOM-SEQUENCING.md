# Ambient headroom audit: sequencing, protocol and persistence

2026-10-24. Audit only, on `merthsloop` working release 2.8.0.58. Production firmware, shared generated headers/build outputs, installer, version, Git index and refs were not changed by this audit. All builds and experiments are under `build/headroom-ambient/sequencing/`. The later parent commit/push does not redefine this audit's frozen baseline.

**No multi-KiB sequencing/protocol/persistence saving was established.** A shared bounded chord-scale walker saves **32 linked image bytes**, with unchanged RAM. It is an optional maintenance cleanup, not enough to fund ambient/drone additions. Packed note-slot compaction saves **zero linked bytes** and should not be integrated. Scene-transaction CRC sharing cannot recover release bytes because that extension is disabled in this target.

## Baseline and exact accounting

The snapshot copies actual current working files, including dirty integrated changes, rather than exporting HEAD. `manifest.json` records SHA-256 hashes of the copied firmware, tools, tests and generated headers. The baseline exactly reproduces the requested **583,388-byte** target. Initial source inputs remain recoverable from the snapshot except `seq.c`, whose exact baseline is retained as `seq.original.c`; `note_ties.original.c` is also retained. `final-source-differences.json` confirms the final snapshot changes only `firmware/src/seq.c` among original firmware inputs.

Prior work reviewed: `ARCHITECTURAL-HEADROOM.md`, `OPTIMIZATION.md`, `REDUNDANCY-AUDIT.md`, `HEADROOM-STORAGE-AUDIT.md`, and the relevant compiler/link/UI/piano-roll audit findings. Shared chord/modifier rules, direct legacy conversion, note-slot deletion, the shared starter renderer, metadata reset and UI entry transactions are already present. Their historical savings are not available again. Previously measured settings/editor/storage sharing and declined CC74 mapping are not presented as new discoveries.

| Private build | Application B | Change B | Flash free B | Main static RAM B | Low RAM B | Pool B |
|---|---:|---:|---:|---:|---:|---:|
| Frozen .58 baseline | 583,388 | — | 948 | 86,176 | 28,928 | 332,876 |
| Final packed-slot compaction, rejected | 583,388 | 0 | 948 | 86,176 | 28,928 | 332,876 |
| Shared chord walker, optional | **583,356** | **−32** | **980** | **86,176** | **28,928** | **332,876** |

Capacity remains 584,336 image bytes, 98,304 main static RAM, 29,496 low RAM and 344,064 pool bytes. Every candidate has zero RAM delta. Main static margin is 12,128 bytes; low RAM margin is 568; pool gap is 11,188, of which 8,192 is the existing reserve (2,996 above reserve). These are linker margins, not stack high-water or hardware deadline measurements. No compression/decoding, factory-content removal or format change is proposed.

The baseline image SHA-256 is `f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`; the optional candidate is `0707e247b2e24c8ea12e2293de164e5ede987e6783136e97715b61d87e52c814`. Exact image/RAM values and hashes are in `measurements.json`.

## Ranked recommendations

### 1. Optional: share the bounded scale walk in ordinary and modified chords

Sources: `firmware/src/seq.c:312` and `:360`. Private patch: `chord-walk.patch`; complete files: `seq.original.c`, `seq.candidate.c`.

`chord_notes_type` and `chord_scale_up` repeat the same increment/test loop, with an identical 48-semitone guard. A small explicit `noinline` helper accepts the already-selected mask and tonic. Each caller keeps its own mask/default selection and CHROM tonic policy. Fixed chord qualities, modifier priority, voice-leading history, inversion/spread, latches, recording and note ownership remain in their existing callers.

The helper is 52 target bytes. `chord_notes_type` shrinks **210 → 152 B**, and `chord_scale_up` **84 → 44 B**. Those symbols save 46 B net, but the **complete linked image saves only 32 B** after layout/alignment. Use the linked number. Main/low/pool RAM are unchanged.

Correctness: an independent side-by-side oracle checks **5,373,952** cases against the original functions: every native scale, all chord qualities, all four quantization modes, roots 0–11 and pitches 0–127, plus extension degrees 0–12 across roots/pitches 0–127. It compares note counts and all four output bytes, including high-note truncation. The actual progression, ties, ownership and phone-step suites also pass. CHR's minor fallback and CHROM's literal-root tonic are retained explicitly.

This adds a shared call at chord-event boundaries (up to four scale walks for a chord), not in per-sample rendering. Target DSP budgets are unchanged; physical key-to-sound/event-burst latency is unmeasured. Upstream conflict burden is **low**: one local helper and two loops, with no saved/wire layout or header changes. Recheck if upstream changes either loop's guard, mask or tonic semantics. Recommendation: retain as a reviewable optional cleanup; do not justify a release or meaningful ambient budget with 32 bytes.

### 2. Ambient/drone implementation: reuse existing note ownership and serialization boundaries

This is an implementation constraint, **not a measured saving**. Generated pitches should use the existing per-track sequence snapshot/direct route and the existing track/project representation. Avoid adding another parallel literal-note list or save encoder merely because ambient playback has a different trigger policy.

Keep the identities that already differ: `seq_harmony` is only **18 target RAM bytes** and owns sequence snapshots; `midi_scale_held` is **256 B**, remembers physical port/channel/key and the sounded pitch; `midi_sel_on` is already nibble-packed at **1,024 B**, remembers destination after selection changes; keyboard/direct/arp latches retain separate release and revoice obligations. These coexist, so they cannot safely share a union. Replacing routing with the 64-entry held-owner table would silently lower the routing capacity. Colliding/clamped pitches, USB reset, channel changes and HOLD/modifier transitions must continue to release the original owner.

For any ambient extension, require exact tests for STOP/panic/track changes, live structural edits during ratchets, independent continuations versus deliberate reattacks, SF_LEG, modifier/latch release, USB/TRS collisions and project/backup round trips before size accounting. Do not use the unmeasured architectural UI estimate as sequencing credit.

## Rejected experiments and larger paths examined

**Packed note-slot metadata compaction:** `note_ties.c:43` shifts `lvl`, `rat` and continuation bits per remaining note. The prototype shifts each packed suffix once and keeps only note-byte movement in the loop. Final `packed-slot.patch` and `note_ties.packed-final.c` preserve unrelated flags and unused continuation bits. **655,360 full-step byte comparisons** cover all 1–4 note counts, valid removed slots, all 256 flags and all 256 packed-level values, with complemented ratchet bytes covering all ratchet values too. This is exhaustive over each individual byte domain, not the Cartesian product of level and ratchet bytes. It matches the original; `note_remove_slot` shrinks 156 → 154 B, but the full image remains **583,388 B**. Reject: no linked headroom.

The first mask drafts shifted unrelated/unused flag bits. The byte oracle caught this; they are invalid and are not candidates. `packed-slot`/`packed-slot-fixed` exploratory outputs and early host logs are retained for provenance; only `packed-slot-final` plus the final oracle support the equivalent rejected result. The final source tree has the original slot-removal implementation.

**Scene transaction/storage CRC:** `scene_transaction.c:9` implements the same CRC polynomial as `storage.c:40`, but `usb.c:23–27` defaults `FELUCCA_SCENE_TX=0`. The .58 target contains no scene-transaction state machine or `sc_crc` symbol. Sharing or deleting it cannot recover release bytes. Its standalone tests pass, but they do not establish live-hook integration. If enabled later, prefer one independent CRC primitive available to both USB and storage; retain standalone/loader compilation and all transaction publication/rollback rules. No linked saving is claimed.

**One generic note owner:** rejected as an immediate headroom change. Keyboard roots/modifier identities, independently held external MIDI pitches, sequence snapshots, direct chord latches and arp sources can be alive simultaneously. The shared harmony header's generic owner API is not proof that replacing all those policies costs less. The target already uses the small sequence-specific snapshot. No additional removable multi-KiB owner implementation was identified.

**Merge selected-note and whole-step edits:** the current `note_chain_action` already shares pitch/place/move logic with no pattern-sized staging buffer. Its validation-before-write passes preserve failure atomicity and original effective predecessors across legacy ties. Whole-step movement additionally carries locks/microtiming, while selected-note edits preserve per-slot levels, ratchets and independent continuation flags. Dropping validation or treating these as the same operation changes behavior. No equivalent linked reduction was established.

**Cache every starter chord/step:** plain and voice-led output already use `sequence_starter_voiced_rhythm` (`sequence_starters.c:101`), shared by Apply and audition. Rewalking preceding chords for voice-leading is real temporary work, but caching requires complete keys for custom degrees/span, rhythm shape, mode, octave, root, scale and voice-leading policy. Preview's mutable lifetime and Apply's complete-pattern generation differ. A cache would add state/invalidation; no target saving or timing benefit was measured. The existing bounded implementation should remain until a concrete candidate proves otherwise.

**Remove legacy formats or old protocol commands:** formats 1–5 remain reachable through `proj_import`, section/autosave fetch and backup restore. Their converters occupy about **1,340 named target function bytes** including the separate v3 initialization helper; this is neither all their dependencies nor removable code. Selected-track STEP versus TRACK_STEP/DRUM_STEP have different addressing, translation and reply obligations. Replies for retired commands 88/89 are deliberate unavailable responses, not retained feature implementations. Keep compatibility.

**Collapse save paths/scratch:** section snapshots, current working-loop autosave, undo and import recovery have different lifetimes. `proj_tmp` is 3,840 B; `proj_slot` is 15,264 B in `.noinit`. They are not proven mutually exclusive. The existing main-loop scratch union is already shared. Removing the later settings load fails because intervening project reads overwrite storage scratch; this was also identified in the prior storage audit. Do not re-count the integrated 2,584-byte legacy intermediate-buffer removal.

**Generic editor schema interpreter:** `ed_handle` is 4,428 target bytes, `ed_live` 1,670, `proj_apply` 1,646, and `events_block` 11,028. These identify review concentrations, not removable duplicates. They contain different IRQ/stop policies, validation, replies, transport and ownership semantics. A descriptor interpreter needs a measured converted command family, full reply/WATCH equivalence and read/write failure tests before a savings claim. No new generic dispatcher or unsubstantiated multi-KiB estimate is recommended.

## Validation and limitations

Pinned toolchain: `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, invoked natively under WSL. The copied `tools/build.py` supplies `-Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, the two-pass preset probe/packer, `FELUCCA_BOOT_TABLES=1`, final `FELUCCA_BOOT_PRESETS=1`, `FELUCCA_SLICE=0`, and the existing target/linker layout. Product/version and copied generated assets are identical across comparisons. `run.py` calls only private `build_app()`/`check()`; it does not invoke normal generation, packaging, loader or installer tools. Preset-generated headers go only to the private copied `source/build/gen`.

Both baseline and chord-walk target checks pass: image entry/ROM-reference/region checks, 925 RAM-text instructions with zero calls, 60-byte CPU1 RAM loop, 6,216-byte zeroed persistent scratch. All **24 target DSP budgets** and **24 hot-function inlining caps** pass; baseline/candidate budget logs are identical. These are static checks, not target cycle or physical deadline measurements.

Final candidate, pinned Zig 0.13 `cc -O2 -UNDEBUG` with private Zig caches: **11 focused programs pass**:

- `note_chain_actions_test`, `seq_note_ties_test`, `sequencing_ownership_test`, `sequencing_phone_step_test`;
- `progression_perform_test`, `sequence_starters_test`;
- `project_test`, `upstream_project_test`, `storage_test`, `project_legacy_conversion_test`, `scene_transaction_test`.

The legacy corpus reports **42,000 import/output/rejection cases**, frozen digest `8b386c47`. The starter suite retains its frozen 100,000-event oracle. Progression tests cover 300 deterministic pad/HOLD/octave transitions with balanced MIDI and real recording/count-in behavior. The initial progression run failed while opening a missing screenshot directory; passing the private output directory fixes the runner, without changing production/tests or assertions. Initial exploratory test logs crossed experiment phases and are not the final-candidate proof: the final runs record the `seq.c` hash per test under `host/chord-walk-final` and `host/chord-walk-compat`.

No full audio golden suite, target-device timing, stack high-water, on-device feel/sound test, physical flash install or phone validation was performed. The oracle and focused tests establish the stated output/state cases, not every possible malformed internal state. The pre-existing `note_chain_actions.c:24` missing-braces warning occurs in baseline and candidate; no budget/golden was relaxed.

## Concrete handoff and reproduction

Deliverable: this report. **No production patch has been applied.** The only candidate worth optional review is `build/headroom-ambient/sequencing/chord-walk.patch` (32 B). Its small return makes deferral reasonable. Reject `packed-slot.patch` for headroom and retain scene support disabled until its separate hook integration is ready. Ambient/drone implementation needs its own measured budget; this scope has supplied only 32 optional bytes.

Evidence under `build/headroom-ambient/sequencing/`:

- `manifest.json`, original/candidate C files, `final-source-differences.json`, `measurements.json`, `scoped-symbols.txt`;
- `source/results/{baseline,chord-walk,packed-slot-final}/`: target ELF/bin/disassembly/symbols/sections/checks;
- `chord-oracle.log` (5,373,952 cases), `slot-oracle.log` (655,360 cases), oracle sources in private `source/tests/`;
- `host/chord-walk-final/`, `host/chord-walk-compat/`: compile/run logs, source hashes and results;
- `*-target_budget.log`, `*-target_inlining_test.log`, `static-validation.json`.

From the private `source/` directory, `python3 run.py <label>` rebuilds its currently selected candidate. For baseline, copy **private** `../seq.original.c` to `firmware/src/seq.c`; for the optional candidate copy **private** `../seq.candidate.c`. Keep original `note_ties.c` for both. Never run these copy commands against the production root. Host runner from the repository root: `python3 build/headroom-ambient/sequencing/test.py <label> <test names...>`; it selects the private tree, compiles with isolated caches and supplies a private screenshot directory. Re-run target checks and tests if rebasing onto newer sources; do not subtract this delta from an unrelated build and call it a measured final image.

